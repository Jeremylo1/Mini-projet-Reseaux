/******************/
/* LABORATOIRE #3 */
/* ISA HERODE     */
/* HERI05539800   */
/******************/

/********************/
/* PROGRAMME CLIENT */
/********************/

/* Programme inspir� de la documentation Microsoft :
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
int CommandMenu();


//FONCTION PRINCIPALE.
int __cdecl main(int argc, char** argv)
{
    setlocale(LC_CTYPE, "fr-FR");  //Pour afficher les accents fran�ais.

    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, * ptr = NULL, hints;

    //const char* sendbuf = "this is a test";
    int iResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    int choice = 0;
    int option = 0;

    //Validation des param�tres.
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

    //R�solution de l'adresse et du port du serveur.
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        std::cerr << "Echec GETADDRINFO - erreur : " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    //Tentative de connexion � une adresse jusqu'� avoir un succ�s.
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        //Cr�ation d'un socket pour se connecteur au serveur.
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

    //Si incapable de se connecter, le socket est d�j� ferm�.
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

    /* SECTION - VALIDATION DES CODES D'ACC�S */
    //Demande de l'identifiant au client.
    std::string id;
    std::cout << std::endl << "Entrez votre identifiant : ";
    std::cin >> id;

    //Demande du mot de passe au client.
    std::string password;
    std::cout << "Entrez votre mot de passe : "; //DO...WHILE V�RIFICATION
    std::cin >> password;

    //Concat�nation des codes d'acc�s.
    std::string accessCode = id + "~" + password;

    //Envoi des codes d'acc�s au serveur.
    iResult = send(ConnectSocket, accessCode.c_str(), accessCode.length(), 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Erreur dans l'envoi des donn�es : " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    //R�ception de la r�ponse du serveur.
    iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Erreur dans la r�ception des donn�es : " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    std::cout << std::endl << "-> R�PONSE DU SERVEUR : " << std::string(buffer, iResult) << std::endl;
    /* FIN DE LA SECTION */

    if (iResult == 14)  //Si l'acc�s est autoris�.
    {
        do  //Le client peut effectuer des actions.
        {
            //Affichage du menu.
            choice = Menu();
            std::string optionNb = std::to_string(choice);

            //Envoi du choix de l'option au serveur.
            iResult = send(ConnectSocket, optionNb.c_str(), optionNb.length(), 0);
            if (iResult == SOCKET_ERROR)
            {
                std::cerr << "Erreur dans l'envoi des donn�es : " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            if (choice == 1)
            {
                /* SECTION - R�CEPTION DE LA LISTE DE FICHIERS */
                //R�ception de la liste de fichiers.
                iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans la r�ception des donn�es : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }
                std::cout << std::endl << "-> LISTE DE FICHIERS : " << std::endl << std::string(buffer, iResult) << std::endl;
                /* FIN DE LA SECTION */

                //Affichage du menu des commandes.
                option = CommandMenu();
                std::string opNb = std::to_string(option);
                
                //Envoi du choix de l'option au serveur.
                iResult = send(ConnectSocket, opNb.c_str(), opNb.length(), 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans l'envoi des donn�es : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                if (option == 1)
                {
                    /* SECTION - COMMANDE */
                    //Demande du nom du fichier � t�l�charger.
                    std::string fileName;
                    std::cout << "Entrez le nom du fichier � t�l�charger : ";
                    std::cin.ignore();
                    std::getline(std::cin, fileName);

                    //Envoi du nom de fichier au serveur.
                    iResult = send(ConnectSocket, fileName.c_str(), fileName.size() + 1, 0);
                    if (iResult == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans l'envoi des donn�es : " << WSAGetLastError() << std::endl;
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                    }

                    //R�ception de la v�rification de l'existence du fichier.
                    iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                    if (iResult == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans la r�ception des donn�es : " << WSAGetLastError() << std::endl;
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                    }

                    if (iResult == 10)  //Si le fichier demand� existe.
                    {
                        //R�ception de la taille du fichier.
                        int fileSize;
                        iResult = recv(ConnectSocket, (char*)&fileSize, sizeof(fileSize), 0);
                        if (iResult == SOCKET_ERROR)
                        {
                            std::cerr << "Erreur dans la r�ception des donn�es : " << WSAGetLastError() << std::endl;
                            closesocket(ConnectSocket);
                            WSACleanup();
                            return 1;
                        }

                        //Ouverture du fichier pour enregistrement (r�pertoire courant).
                        std::ofstream file;
                        file.open(fileName, std::ios::binary);

                        //V�rification de l'ouverture du fichier.
                        if (!file.is_open())
                        {
                            std::cerr << "Impossible d'ouvrir le fichier pour enregistrement !" << std::endl;
                            closesocket(ConnectSocket);
                            WSACleanup();
                            return 1;
                        }

                        //R�ception du fichier par parties.
                        int receivedBytes;
                        int totalBytesReceived = 0;

                        while ((totalBytesReceived < fileSize))
                        {
                            receivedBytes = recv(ConnectSocket, recvbuf, recvbuflen, 0);
                            if (receivedBytes == SOCKET_ERROR)
                            {
                                std::cerr << "Erreur dans la r�ception des donn�es : " << WSAGetLastError() << std::endl;
                                closesocket(ConnectSocket);
                                WSACleanup();
                                return 1;
                            }

                            //�criture des donn�es dans le fichier.
                            file.write(recvbuf, receivedBytes);

                            //Mise � jour du nombre total de bytes re�us.
                            totalBytesReceived += receivedBytes;
                        }

                        //V�rifie que toutes les donn�es ont �t� re�ues.
                        if (totalBytesReceived != fileSize)
                        {
                            std::cerr << "Erreur : Taille du fichier re�ue diff�rente de la taille attendue !" << std::endl;
                            closesocket(ConnectSocket);
                            WSACleanup();
                            return 1;
                        }
                        else
                        {
                            std::cout << std::endl << "-> FICHIER T�L�CHARG� AVEC SUCC�S !" << std::endl;
                            std::cout << "-> Localisation : dans le r�pertoire courant." << std::endl;
                        }

                        //Fermeture du fichier.
                        file.close(); 
                    }
                    else
                    {
                        std::cerr << std::endl << "-> FICHIER DEMAND� INEXISTANT !" << std::endl;
                    }
                    /* FIN DE LA SECTION */
                }
                else if (choice == 3)
                {
                    // Affiche et envoie la commande au serveur
                    std::string command;
                    std::cout << "Entrez la commande � ex�cuter sur le serveur: ";
                    std::cin.ignore();
                    std::getline(std::cin, command);

                    // Envoi de la commande au serveur
                    iResult = send(ConnectSocket, command.c_str(), command.length(), 0);
                    if (iResult == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans l'envoi des donn�es : " << WSAGetLastError() << std::endl;
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                    }

                    // R�ception de la r�ponse du serveur
                    iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                    if (iResult == SOCKET_ERROR)
                    {
                        std::cerr << "Erreur dans la r�ception des donn�es : " << WSAGetLastError() << std::endl;
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                    }
                    std::cout << std::endl << "-> R�PONSE DU SERVEUR : " << std::endl << std::string(buffer, iResult) << std::endl;
                    /* FIN DE LA SECTION */
                }
            }
            else if (choice == 3)
            {
                // Affiche et envoie la commande au serveur
                std::string command;
                std::cout << "Entrez la commande � ex�cuter sur le serveur: ";
                std::cin.ignore();
                std::getline(std::cin, command);
                
                std::cout << "commande : " << command << std::endl;

                // Envoi de la commande au serveur
                iResult = send(ConnectSocket, command.c_str(), command.length(), 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans l'envoi des donn�es : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }

                // R�ception de la r�ponse du serveur
                memset(buffer, 0, BUFFER_SIZE);
                iResult = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
                if (iResult == SOCKET_ERROR)
                {
                    std::cerr << "Erreur dans la r�ception des donn�es : " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }
                std::cout << std::endl << "-> R�PONSE DU SERVEUR : " << std::endl << std::string(buffer, iResult) << std::endl;
            }

        } while (choice != 2);
    }

    //D�connexion du client.
    std::cout << std::endl << "-> ARR�T DE LA CONNEXION ..." << std::endl;
    
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Echec SHUTDOWN - erreur : " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    //Lib�ration des ressources.
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
    std::cout << "[1] Afficher la liste de fichiers disponibles" << std::endl;
    std::cout << "[2] Se d�connecter" << std::endl;
    std::cout << "[3] Ex�cuter une commande Windows" << std::endl;
    std::cout << "*********************************************" << std::endl << std::endl;

    do
    {
        std::cout << "Entrez le num�ro de votre choix (1, 2 ou 3) : ";
        std::cin >> option;

        try  //Conversion en entier et gestion d'erreur.
        {
            nb = std::stoi(option);

            if ((nb == 1) || (nb == 2) || (nb == 3))
            {
                //Choix valide.
                validArg = true;
            }
            else
            {
                std::cerr << "NOMBRE INVALIDE ! Tapez 1, 2 ou 3." << std::endl;
            }
        }
        catch (const std::invalid_argument& ex)
        {
            std::cerr << "ENTR�E INVALIDE ! Tapez 1, 2 ou 3." << std::endl;
        }
    } while (!validArg);

    return nb;
}


//Affiche le menu des commandes au client.
int CommandMenu()
{
    std::string option;
    int nb;
    bool validArg = false;

    std::cout << "****************************************" << std::endl;
    std::cout << "Choisissez l'une des options suivantes : " << std::endl;
    std::cout << "[1] Effectuer une commande" << std::endl;
    std::cout << "[2] Revenir au menu principal" << std::endl;
    std::cout << "[3] Ex�cuter une commande Windows" << std::endl;
    std::cout << "****************************************" << std::endl << std::endl;

    do 
    {
        std::cout << "Entrez le num�ro de votre choix (1, 2 ou 3) : ";
        std::cin >> option;

        try  //Conversion en entier et gestion d'erreur.
        {
            nb = std::stoi(option);
            
            if ((nb == 1) || (nb == 2) || (nb == 3))
            {
                //Choix valide.
                validArg = true;
            }
            else 
            {
                std::cerr << "NOMBRE INVALIDE ! Tapez 1, 2 ou 3." << std::endl;
            }
        }
        catch (const std::invalid_argument& ex) 
        {
            std::cerr << "ENTR�E INVALIDE ! Tapez 1, 2 ou 3." << std::endl;
        }
    } while (!validArg);

    return nb;
}