/************************************/
/* Mini-Projet Conception Client    */
/* ISA HERODE - HERI05539800        */
/* JEREMY LE TOULLEC - LETJ14039804 */
/************************************/

/********************/
/* PROGRAMME CLIENT */
/********************/

/* Programme inspiré de la documentation Microsoft :
https://learn.microsoft.com/fr-fr/windows/win32/winsock/complete-client-code */

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

//Librairies.
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <string>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

//Prototypes.
int Menu();


//FONCTION PRINCIPALE.
int __cdecl main(int argc, char** argv)
{
    setlocale(LC_CTYPE, "fr-FR");  //Pour afficher les accents français.

    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, * ptr = NULL, hints;

    int iResult;
    int ackResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    int choice = 0;

    /* DEBUT CODE MICROSOFT */
    //Validation des paramètres.
    if (argc != 2)
    {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
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
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //Résolution de l'adresse et du port du serveur.
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        std::cerr << "Echec GETADDRINFO - erreur : " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    //Tentative de connexion à une adresse jusqu'à avoir un succès.
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        //Création d'un socket pour se connecteur au serveur.
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET)  //Pour s'assurer que le socket est valide.
        {
            std::cerr << "Echec SOCKET - erreur : " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        //Connection au serveur.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    //Si incapable de se connecter, le socket est déjà fermé.
    if (ConnectSocket == INVALID_SOCKET)
    {
        std::cerr << std::endl << "-> IMPOSSIBLE DE SE CONNECTER AU SERVEUR !" << std::endl;
        WSACleanup();
        return 1;
    }
    else
    {
        std::cout << std::endl << "-> CONNEXION AU SERVEUR ..." << std::endl;
    }
    /* FIN CODE MICROSOFT */

    //Accès au serveur.
    do  //Le client peut effectuer des actions ...
    {
        //Affichage du menu.
        choice = Menu();
        std::string optionNb = std::to_string(choice);

        //Envoi du choix de l'option au serveur (menu principal).
        iResult = send(ConnectSocket, optionNb.c_str(), optionNb.length(), 0);
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        //ACK.
        ackResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
        if (ackResult == SOCKET_ERROR)
        {
			std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

        /* SECTION - TÉLÉCHARGER UN FICHIER */
        if (choice == 1)
        {
            //Demande du nom du fichier à télécharger.
            std::string fileName;
            std::cout << "Entrez le nom du fichier (du répertoire courant) à télécharger : ";
            std::cin.ignore();
            std::getline(std::cin, fileName);

            //Envoi du nom de fichier à télécharger au serveur.
            iResult = send(ConnectSocket, fileName.c_str(), fileName.size() + 1, 0);
            if (iResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            //ACK.
            ackResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
            if (ackResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            //Réception de la vérification de l'existence du fichier.
            iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
            if (iResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            //ACK
            ackResult = send(ConnectSocket, "OK", 2, 0);
            if (ackResult == SOCKET_ERROR)
            {
				std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}

            if (iResult == 10)  //Si le fichier demandé existe.
            {
                //Réception de la taille du fichier.
                int fileSize;
                iResult = recv(ConnectSocket, (char*)&fileSize, sizeof(fileSize), 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //ACK
                ackResult = send(ConnectSocket, "OK", 2, 0);
                if (ackResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //Ouverture du fichier pour enregistrement (répertoire courant).
                std::ofstream file;
                file.open(fileName, std::ios::binary);

                //Vérification de l'ouverture du fichier.
                if (!file.is_open())
                {
                    std::cerr << "Impossible d'ouvrir le fichier pour enregistrement !" << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //Réception du fichier par parties.
                int receivedBytes;
                int totalBytesReceived = 0;

                while ((totalBytesReceived < fileSize))
                {
                    receivedBytes = recv(ConnectSocket, recvbuf, recvbuflen, 0);
                    if (receivedBytes == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                    }

                    //Écriture des données dans le fichier.
                    file.write(recvbuf, receivedBytes);

                    //Mise à jour du nombre total de bytes reçus.
                    totalBytesReceived += receivedBytes;
                }

                //Vérifie que toutes les données ont été reçues.
                if (totalBytesReceived != fileSize)
                {
                    std::cerr << "Erreur : Taille du fichier reçue différente de la taille attendue !" << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }
                else
                {
                    //ACK
                    ackResult = send(ConnectSocket, "OK", 2, 0);
                    if (ackResult == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                    }

                    std::cout << std::endl << "-> FICHIER TÉLÉCHARGÉ AVEC SUCCÈS !" << std::endl;
                    std::cout << "-> Localisation : dans le répertoire courant." << std::endl;
                }

                //Fermeture du fichier.
                file.close();
            }
            else
            {
                std::cerr << std::endl << "-> FICHIER DEMANDÉ INEXISTANT !" << std::endl;
            }
        }
        /* FIN DE LA SECTION */

        /* SECTION - TRANSMETTRE UN FICHIER */
        if (choice == 2)
        {
            //Demande du nom du fichier à transmettre.
            std::string fileExportName;
            std::cout << "Entrez le nom du fichier (du répertoire courant) à transmettre : ";
            std::cin.ignore();
            std::getline(std::cin, fileExportName);

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

            //Vérification de l'existence du fichier demandé.
            size_t found = fileListChecked.find("#" + fileExportName + "~");

            //Le fichier existe.
            if (found != std::string::npos)
            {
                //Envoi d'un signal positif au serveur (le fichier existe).
                iResult = send(ConnectSocket, "NOM VALIDE", 10, 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //ACK.
                ackResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (ackResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                std::cout << "-> NOM DE FICHIER VALIDE !" << std::endl;

                //Envoi du nom de fichier à transmettre au serveur.
                iResult = send(ConnectSocket, fileExportName.c_str(), fileExportName.size() + 1, 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //ACK.
                ackResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (ackResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //Ouverture du fichier (situé dans le répertoire courant).
                std::ifstream file(fileExportName, std::ios::binary);
                if (!file.is_open())
                {
                    std::cerr << "Impossible d'ouvrir le fichier demandé !" << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //Récupération de la taille du fichier.
                file.seekg(0, std::ios::end);
                int fileExpSize = file.tellg();
                file.seekg(0, std::ios::beg);

                //Envoi de la taille du fichier au serveur.
                iResult = send(ConnectSocket, (char*)&fileExpSize, sizeof(fileExpSize), 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //ACK.
                ackResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (ackResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //Envoi du fichier par parties.
                int sentBytes;
                int totalBytesSent = 0;

                while ((totalBytesSent < fileExpSize))
                {
                    file.read(recvbuf, recvbuflen);

                    //Envoi des données au serveur.
                    sentBytes = send(ConnectSocket, recvbuf, file.gcount(), 0);
                    if (sentBytes == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                    }

                    //Mise à jour du nombre total de bytes envoyés.
                    totalBytesSent += sentBytes;
                }

                //ACK.
                ackResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (ackResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                std::cout << "-> FICHIER ENVOYÉ AU SERVEUR AVEC SUCCÈS !" << std::endl;

                // Fermeture du fichier.
                file.close();
            }
            //Le fichier n'existe pas.
            else
            {
                iResult = send(ConnectSocket, "NOM INVALIDE", 12, 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                //ACK.
                ackResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (ackResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                std::cout << "-> FICHIER À TRANSMETTRE INEXISTANT !" << std::endl;
            }
        }
        /* FIN DE LA SECTION */

        /* SECTION - COMMANDE WINDOWS */
        if (choice == 3)
        {
            // Affiche et envoie la commande au serveur
            std::string command;
            std::cout << "Entrez la commande à exécuter sur le serveur: ";
            std::cin.ignore();
            std::getline(std::cin, command);

            std::cout << std::endl << "-> COMMANDE : " << std::endl << command << std::endl;

            // Envoi de la commande au serveur
            iResult = send(ConnectSocket, command.c_str(), command.length(), 0);
            if (iResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            //ACK.
            ackResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
            if (ackResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            //Réception du résultat de la commande.
            memset(buffer, 0, BUFFER_SIZE);
            iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
            if (iResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            //ACK
            ackResult = send(ConnectSocket, "OK", 2, 0);
            if (ackResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur ACK : " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            std::cout << "-> RÉPONSE DU SERVEUR : " << std::endl << std::string(buffer, iResult) << std::endl;
        }
        /* FIN DE LA SECTION */

    } while (choice != 4);  //... Tant qu'il ne se déconnecte pas.


    //Déconnexion du client.
    std::cout << std::endl << "-> ARRÊT DE LA CONNEXION ..." << std::endl;

    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Echec SHUTDOWN - erreur : " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }


    //Libération des ressources.
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}




//Affiche le menu principal au client.
int Menu()
{
    std::string option;
    int nb;
    bool validArg = false;

    std::cout << std::endl;
    std::cout << "*********************************************" << std::endl;
    std::cout << "             * MENU PRINCIPAL *              " << std::endl;
    std::cout << "[1] Télécharger un fichier" << std::endl;
    std::cout << "[2] Transmettre un fichier au serveur" << std::endl;
    std::cout << "[3] Exécuter une commande Windows" << std::endl;
    std::cout << "[4] Se déconnecter" << std::endl;
    std::cout << "*********************************************" << std::endl << std::endl;

    do
    {
        std::cout << "Entrez le numéro de votre choix (1, 2, 3 ou 4) : ";
        std::cin >> option;

        try  //Conversion en entier et gestion d'erreur.
        {
            nb = std::stoi(option);

            if ((nb == 1) || (nb == 2) || (nb == 3) || (nb == 4))
            {
                //Choix valide.
                validArg = true;
            }
            else
            {
                std::cerr << "NOMBRE INVALIDE ! Tapez 1, 2, 3 ou 4." << std::endl;
            }
        }
        catch (const std::invalid_argument& ex)
        {
            std::cerr << "ENTRÉE INVALIDE ! Tapez 1, 2, 3 ou 4." << std::endl;
        }
    } while (!validArg);

    return nb;
}
