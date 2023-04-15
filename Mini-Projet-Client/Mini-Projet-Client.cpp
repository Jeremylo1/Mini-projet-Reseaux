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
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    int choice = 0;
    int option = 0;

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
        std::cout << std::endl << "-> TENTATIVE DE CONNEXION" << std::endl;
    }

    /* SECTION - VALIDATION DES CODES D'ACCÈS */
    //Demande de l'identifiant au client.
    std::string id;
    std::cout << std::endl << "Entrez votre identifiant : ";
    std::cin >> id;

    //Demande du mot de passe au client.
    std::string password;
    std::cout << "Entrez votre mot de passe : ";
    std::cin >> password;

    //Concaténation des codes d'accès.
    std::string accessCode = id + "~" + password;

    //Envoi des codes d'accès au serveur.
    iResult = send(ConnectSocket, accessCode.c_str(), accessCode.length(), 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    //Réception de la réponse du serveur.
    iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    std::cout << std::endl << "-> RÉPONSE DU SERVEUR : " << std::string(buffer, iResult) << std::endl;
    /* FIN DE LA SECTION */

    if (iResult == 14)  //Si l'accès est autorisé.
    {
        do  //Le client peut effectuer des actions.
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

                //Réception de la vérification de l'existence du fichier.
                iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
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
            /*if (choice == 2)
            {

            }*/
            /* FIN DE LA SECTION */

            /* SECTION - COMMANDE WINDOWS */
            if (choice == 3)
            {
                // Affiche et envoie la commande au serveur
                std::string command;
                std::cout << "Entrez la commande à exécuter sur le serveur: ";
                std::cin.ignore();
                std::getline(std::cin, command);

                std::cout << "commande : " << command << std::endl;

                // Envoi de la commande au serveur
                iResult = send(ConnectSocket, command.c_str(), command.length(), 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans l'envoi des données : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                // Réception de la réponse du serveur
                memset(buffer, 0, BUFFER_SIZE);
                iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans la réception des données : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }
                std::cout << std::endl << "-> RÉPONSE DU SERVEUR : " << std::endl << std::string(buffer, iResult) << std::endl;
            }
            /* FIN DE LA SECTION */

        } while (choice != 4);
    }

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

    /*std::cout << std::endl;
    std::cout << "*********************************************" << std::endl;
    std::cout << "             * MENU PRINCIPAL *              " << std::endl;
    std::cout << "[1] Afficher la liste de fichiers disponibles" << std::endl;
    std::cout << "[2] Se déconnecter" << std::endl;
    std::cout << "[3] Exécuter une commande Windows" << std::endl;
    std::cout << "*********************************************" << std::endl << std::endl;*/

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
