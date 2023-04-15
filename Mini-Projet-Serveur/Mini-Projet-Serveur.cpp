/************************************/
/* Mini-Projet Conception Serveur   */
/* ISA HERODE - HERI05539800        */
/* JEREMY LE TOULLEC - LETJ14039804 */
/************************************/

/*********************/
/* PROGRAMME SERVEUR */
/*********************/

/* Programme inspiré de la documentation Microsoft :
https://learn.microsoft.com/fr-fr/windows/win32/winsock/complete-server-code */

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

//Librairies.
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <direct.h>
#include <filesystem>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

//Prototypes.
void executeWindowsCommand(const std::string& cmd, std::string& output, std::string& currentDirectory);


//FONCTION PRINCIPALE.
int __cdecl main(void)
{
    setlocale(LC_CTYPE, "fr-FR");  //Pour afficher les accents français.

    WSADATA wsaData;
    int iResult;
    int iSendResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];

    bool checkedCode = false;
    int choice = 0;
    int option = 0;

    std::string currentDirectory;

    // Obtenir le répertoire courant et le stocker dans currentDirectory (pour les commandes "cd").
    char directorySize[FILENAME_MAX];
    if (_getcwd(directorySize, FILENAME_MAX))
    {
        currentDirectory = directorySize;

        // Supprimer le caractère de nouvelle ligne à la fin de currentDirectory, s'il est présent.
        if (!currentDirectory.empty() && currentDirectory.back() == '\n')
        {
            currentDirectory.pop_back();
        }
    }

    //Initialisation de Winsock.
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        std::cerr << "Echec de WSAStartup - erreur : " << iResult << std::endl;
        return 1;
    }

    //Socket de flux pour le protocole TCP.
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;  //Famille d’adresses IPv4.
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    //Résolution de l'adresse et du port du serveur.
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        std::cerr << "Echec GETADDRINFO - erreur : " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    //Création d'un socket pour que le serveur écoute les connexions clientes.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)  //Pour s'assurer que le socket est valide.
    {
        std::cerr << "Echec SOCKET - erreur : " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    //Liaison du socket à une adresse IP et un port.
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Echec BIND - erreur : " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);  //Libération de la mémoire allouée.

    while (true)
    {
        std::cout << std::endl << "-> EN ATTENTE D'UNE CONNEXION ..." << std::endl;  //À enlever !!!

        //Écoute sur le socket pour les demandes de connexion entrantes.
        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "Echec LISTEN - erreur : " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        //Accepter la connexion d'un client sur le socket.
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET)
        {
            std::cerr << "Echec ACCEPT - erreur : " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "---> CONNEXION D'UN CLIENT" << std::endl;  //À enlever !!!

        //Le client a accès au serveur.
        do
        {
            //Réception du choix de l'option du client (menu principal).
            iResult = recv(ClientSocket, buffer, BUFFER_SIZE, 0);
            if (iResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            //Conversion en entier.
            choice = std::stoi(std::string(buffer, iResult));

            /* SECTION - TÉLÉCHARGER UN FICHIER */
            if (choice == 1)
            {
                std::cout << "---> OPTION DE TÉLÉCHARGEMENT CHOISI" << std::endl;  //À enlever !!!

                //On fait une liste de fichiers du répertoire courant.
                std::string fileListChecked = "";  //Pour le processus de validation de nom de fichier.
                WIN32_FIND_DATA fileData;
                HANDLE hFind = FindFirstFile(("*"), &fileData);

                if (hFind != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                        {
                            fileListChecked += "#";
                            fileListChecked += fileData.cFileName;
                            fileListChecked += "~";
                        }
                    } while (FindNextFile(hFind, &fileData));
                    FindClose(hFind);
                }

                //Réception du nom de fichier à télécharger.
                int receivedBytes = recv(ClientSocket, buffer, BUFFER_SIZE, 0);
                if (receivedBytes <= 0)
                {
                    std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 1;
                }

                std::string fileName(buffer);

                //Vérification de l'existence du fichier demandé.
                size_t found = fileListChecked.find("#" + fileName + "~");

                //Le fichier existe.
                if (found != std::string::npos)
                {
                    iResult = send(ClientSocket, "NOM VALIDE", 10, 0);
                    if (iResult == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                    std::cout << "-----> NOM DE FICHIER VALIDE" << std::endl;  //À enlever !!!

                    //Ouverture du fichier (situé dans le répertoire courant).
                    std::ifstream file(fileName, std::ios::binary);
                    if (!file.is_open())
                    {
                        std::cerr << "Impossible d'ouvrir le fichier demandé !" << std::endl;
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }

                    //Récupération de la taille du fichier.
                    file.seekg(0, std::ios::end);
                    int fileSize = file.tellg();
                    file.seekg(0, std::ios::beg);

                    //Envoi de la taille du fichier au client.
                    iResult = send(ClientSocket, (char*)&fileSize, sizeof(fileSize), 0);
                    if (iResult == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }

                    //Envoi du fichier par parties.
                    int sentBytes;
                    int totalBytesSent = 0;

                    while ((totalBytesSent < fileSize))
                    {
                        file.read(recvbuf, recvbuflen);

                        //Envoi des données au client.
                        sentBytes = send(ClientSocket, recvbuf, file.gcount(), 0);
                        if (sentBytes == SOCKET_ERROR)
                        {
                            std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                            closesocket(ClientSocket);
                            WSACleanup();
                            return 1;
                        }

                        //Mise à jour du nombre total de bytes envoyés.
                        totalBytesSent += sentBytes;
                    }
                    std::cout << "-----> FICHIER ENVOYÉ AU CLIENT" << std::endl;  //À enlever !!!

                    // Fermeture du fichier.
                    file.close();
                }
                //Le fichier n'existe pas.
                else
                {
                    iResult = send(ClientSocket, "NOM INVALIDE", 12, 0);
                    if (iResult == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                    std::cout << "-----> NOM DE FICHIER INVALIDE" << std::endl;  //À enlever !!!
                }
            }
            /* FIN DE LA SECTION */

            /* SECTION - TRANSMETTRE UN FICHIER */
            /*if (choice == 2)
            {
                std::cout << "---> OPTION DE TÉLÉVERSEMENT CHOISI ..." << std::endl;
                    
            }*/
            /* FIN DE LA SECTION */

            /* SECTION - COMMANDE WINDOWS */
            if (choice == 3)
            {
                std::cout << "---> OPTION DE COMMANDE WINDOWS CHOISI ..." << std::endl;  //À enlever !!!

                //Réinitialisation du buffer.
                memset(buffer, 0, BUFFER_SIZE);

                //Réception de la commande du client.
                iResult = recv(ClientSocket, buffer, BUFFER_SIZE, 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 1;
                }
                std::string command(buffer, iResult);

                //Exécution de la commande.
                std::string commandOutput;
                executeWindowsCommand(command, commandOutput, currentDirectory);

                //Envoi du résultat de la commande au client.
                iResult = send(ClientSocket, commandOutput.c_str(), commandOutput.length(), 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 1;
                }
            }
            /* FIN DE LA SECTION */

        } while (choice != 4);


        //Déconnexion du serveur.
        std::cout << "---> DÉCONNEXION DU CLIENT" << std::endl;  //À enlever !!!

        iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "Echec SHUTDOWN - erreur : " << WSAGetLastError() << std::endl;
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
    }


    //Libération des ressources.
    closesocket(ListenSocket);
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}




//Permet d'exécuter des commandes Windows.
void executeWindowsCommand(const std::string& cmd, std::string& output, std::string& currentDirectory)
{
    std::string commandToExecute;

    // Utiliser le répertoire courant
    commandToExecute = "cd " + currentDirectory;

    // Si la commande est "cd", ajoutez " && echo %cd%"
    // Si la commande est "cd", changez le répertoire de travail.
    if (cmd.substr(0, 2) == "cd")
    {
        std::string newDir = cmd.substr(3);
        if (newDir.back() == '\n') {
            newDir.pop_back(); // Supprimez le caractère de nouvelle ligne, s'il y en a un.
        }
        try
        {
            std::filesystem::current_path(newDir);
            currentDirectory = std::filesystem::current_path().string();
            output = currentDirectory;
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            output = "Erreur : Impossible de changer le répertoire de travail.\n";
        }
    }
    else
    {
        commandToExecute += cmd;
    }

    std::cout << commandToExecute << std::endl;
    std::array<char, 128> buffer;
    std::shared_ptr<FILE> pipe(_popen(commandToExecute.c_str(), "r"), _pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get()))
    {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            output += buffer.data();
    }

    // Si la commande était "cd", mettre à jour le répertoire courant
    if (cmd.substr(0, 2) == "cd")
    {
        currentDirectory = output;

        // Supprimer le caractère de nouvelle ligne à la fin de currentDirectory, s'il est présent
        if (!currentDirectory.empty() && currentDirectory.back() == '\n')
        {
            currentDirectory.pop_back();
        }
    }

    //Afficher la sortie de la commande.
    std::cout << output << std::endl;
}