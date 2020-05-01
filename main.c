#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <time.h>

#include "se_fichier.h"

typedef struct 
{
	int nbrFrames;		//< Nombre de frames
	int taillePage;		//< Taille d'une page mémoire s
	int nbrPage;		//< Nombre de pages disponibles en mémoire lente n
	int nbrThread;		//< Nombre de threads fils
	int nbrAcces;		//< Nombre d'acces demandes par thread
	
} DATA;

typedef struct
{
	int *memRapide;		//< Memoire physique RAM
	int *memLente;		//< Memoire physique Disque
	int *tabIndex;		//< Index pour le LRU
	
} MEMORY;

typedef struct 
{
	pthread_mutex_t *mut;
	
	int hit;		//< Nombre de hit
	int numReq;		//< Nombre de requête
	int nbrPages;	//< Nombre de page
	
} ARG_T;

//Fonction qui permet de recuper les informations
//du fichier de configuration et de les stocker 
//dans la structure DATA
DATA initStruct(DATA dt, const char *chemin)
{
	int rd;
	
	SE_FICHIER file;
	
	file = SE_ouverture(chemin, O_RDONLY);
	if (file.descripteur < 0) 
	{
		fprintf(stderr, "\nERR: opening cfg file\n");
		exit(EXIT_FAILURE);
	}

	rd = SE_lectureEntier(file, &dt.nbrFrames);
	if (rd <= 0)
	{
		fprintf(stderr, "\nERR: missing nbrFrames\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}

	rd = SE_lectureEntier(file, &dt.taillePage);
	if (rd <= 0)
	{
		fprintf(stderr, "\nERR: missing taillePage\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrPage);
	if (rd <= 0)
	{
		fprintf(stderr, "\nERR: missing nbrPage\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrThread);
	if (rd <= 0)
	{
		fprintf(stderr, "\nERR: missing nbrThread\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrAcces);
	if (rd <= 0)
	{
		fprintf(stderr, "\nERR: missing nbrAcces\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	SE_fermeture(file);
	
	return dt;
}

//Fonction qui alloue la mémoire pour la mémoire lente,
//la mémoire rapide et l'index
MEMORY initMemoirePhysique(MEMORY mem, int nbrPage, int nbrFrames)
{
	mem.memLente = malloc(nbrPage * sizeof(int));
	for(int size = 0; size < nbrPage; size++)
		mem.memLente[size] = -1;
	
	mem.memRapide = malloc(nbrFrames * sizeof(int));
	for(int size = 0; size < nbrFrames; size++)
		mem.memRapide[size] = -1;
	
	mem.tabIndex = malloc(nbrFrames * sizeof(int));
	for(int size = 0; size < nbrFrames; size++)
		mem.tabIndex[size] = -1;
		
	return mem;
}

void afficheConfig(DATA dt)
{
	printf("Nombre de frame : %d\n", dt.nbrFrames);
	printf("Taille d'une page : %d = %dko\n", dt.taillePage, (dt.taillePage/1000));
	printf("Nombre de page dans la memoire lente : %d\n", dt.nbrPage);
	printf("Nombre de thread : %d\n", dt.nbrThread);
	printf("Nombre de demande par thread : %d\n\n", dt.nbrAcces);
}


int LRU(int nbrFrame, int nbrPage, int taillePage, MEMORY *mem, int adresse)
{
	int valMax = 0;
	
	//Recheche de l'adresse dans la mémoire Rapide
	for(int cmptR = 0; cmptR < nbrFrame; cmptR++)
	{
		//Si la case mémoire est vide
		if(mem -> memRapide[cmptR] == -1)
		{
			mem -> memRapide[cmptR] = adresse;
			
			for(int cmptI = 0; cmptI < nbrFrame; cmptI++)
			{
				if(mem -> tabIndex[cmptI] != -1 && cmptI != cmptR)
					mem -> tabIndex[cmptI] += 1;
				
				else if(cmptI == cmptR)
					mem -> tabIndex[cmptI] = 0;
			}
			
			return cmptR * taillePage + (adresse % taillePage); //On return l'adresse logique
		}
		
		//Si la case mémoire contient la valeur de l'adresse
		else if(mem -> memRapide[cmptR] == adresse)
		{
			for(int cmptI = 0; cmptI < nbrFrame; cmptI++)
			{
				if(mem -> tabIndex[cmptI] != -1 && cmptI != cmptR)
					mem -> tabIndex[cmptI] += 1;
				
				else if(cmptI == cmptR)
					mem -> tabIndex[cmptI] = 0;
			}
			
			return cmptR * taillePage + (adresse % taillePage); //On return l'adresse logique
		}
	}
	
	//On remplis la mémoire lente avec l'adresse si la mémoire lente ne contiens pas déjà cette adresse
	for(int cmptL = 0; cmptL < nbrPage; cmptL++)
	{
		if(mem -> memLente[cmptL] != adresse || mem -> memLente[cmptL] == -1)
		{
			mem -> memLente[cmptL] = adresse;
			break;
		}
	}
	
	//Recherche dans la memoire lente
	for(int cmptL = 0; cmptL < nbrPage; cmptL++)
	{
		//Si la case de la mémoire lente contient l'adresse
		if(mem -> memLente[cmptL] == adresse)
		{
			//Recherche de la postion du plus ancienne
			for(int cmptI = 0; cmptI < nbrFrame; cmptI++)
			{
				if(mem -> tabIndex[cmptI] > valMax)
					valMax = mem -> tabIndex[cmptI];
			}
			
			//Modification du cache switch des valeurs de la case memLente avec la case de memRapide
			for(int cmptR = 0; cmptR < nbrFrame; cmptR++)
			{
				if(mem -> tabIndex[cmptR] == valMax)
				{
					mem -> memLente[cmptL] = mem -> memRapide[cmptR];
					mem -> memRapide[cmptR] = adresse;
					
					//Mise à jour de l'index
					for(int cmptI = 0; cmptI < nbrFrame; cmptI++)
					{
						if(mem -> tabIndex[cmptI] != -1 && cmptI != cmptR)
							mem -> tabIndex[cmptI] += 1;
						
						else if(cmptI == cmptR)
							mem -> tabIndex[cmptI] = 0;
					}
					
					return -1; // 
				}	
			}
		}
	}
	
	return -1;
}

void creationTube()
{
	int valFIFO;
	
	valFIFO = mkfifo("/tmp/FIFO1", 0666);
	if(valFIFO != 0)
	{
		fprintf(stderr, "\nERR: creating fifo1\n");
		exit(EXIT_FAILURE);
	}
	
	printf("Création de FIFO1\n");
	
	valFIFO = mkfifo("/tmp/FIFO2", 0666);
	if(valFIFO != 0)
	{
		fprintf(stderr, "\nERR: creating fifo2\n");
		exit(EXIT_FAILURE);
	}
	
	printf("Création de FIFO2\n\n");
}

void suppressionTube()
{
	unlink("/tmp/FIFO1");
	printf("\nSupression de FIFO1\n");
	
	unlink("/tmp/FIFO2");
	printf("Supression de FIFO2\n\n");
}

int lectureTube(const char *chemin)
{
	int valRD, nb_numbers;
	
	SE_FICHIER tube = SE_ouverture(chemin, O_RDONLY);
	if (tube.descripteur < 0) 
	{
		fprintf(stderr, "\nERR: opening pipe\n");
		exit(EXIT_FAILURE);
	}
	
	valRD = SE_lectureEntier(tube, &nb_numbers);
	if (valRD <= 0) 
	{
		fprintf(stderr, "\nERR: read in pipe\n");
		SE_fermeture(tube);
		unlink(chemin);
		exit(EXIT_FAILURE);
	}
	
	SE_fermeture(tube);
	
	return nb_numbers;
}

void ecritureTube(const char *chemin, int val)
{
	int valWR;
	
	SE_FICHIER tube = SE_ouverture(chemin, O_WRONLY);
	if (tube.descripteur < 0) 
	{
		fprintf(stderr, "\nERR: opening pipe\n");
		exit(EXIT_FAILURE);
	}
	
	valWR = SE_ecritureEntier(tube, val);
	if (valWR <= 0) 
	{
		fprintf(stderr, "\nERR: write in pipe\n");
		SE_fermeture(tube);
		unlink(chemin);
		exit(EXIT_FAILURE);
	}
	
	valWR = SE_ecritureCaractere (tube, ' ');
	if(valWR <= 0)
	{
		fprintf(stderr, "\nERR: write in pipe\n");
		SE_fermeture(tube);
		unlink(chemin);
		exit(EXIT_FAILURE);
	}
	
	SE_fermeture (tube);
}

void * demandeAcces (void * arg)
{
	ARG_T *at = (ARG_T *) arg;
	
	//Création du chemin pour le tube
	const char *chemin1 = "/tmp/FIFO1";
	const char *chemin2 = "/tmp/FIFO2";
	
	int reponse;
	int page;
	
	printf("debut du thread fils : %ld\n", pthread_self());
	
	for(int cmpt = 0; cmpt < at -> numReq; cmpt++)
	{
		//Mutex
		pthread_mutex_lock(at -> mut);
		
		page = rand() % 10;
		printf("Le thread n°%ld fais une demande pour la page : %d\n", pthread_self(), page);
		
		ecritureTube(chemin1, page);
		
		reponse = lectureTube(chemin2);
		printf("l'adresse logique donné par le père est : %d\n\n", reponse);
		
		if(reponse != -1)
			at -> hit += 1;
				
		//Fin du mutex
		pthread_mutex_unlock(at -> mut);
	}
	
	return NULL;
}

void gestionThread(DATA dt)
{
	pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
	pthread_t * tid;
	
	ARG_T *at;
	MEMORY mem;
	
	int adresse, page;
	int moyenneHit = 0;
	
	const char *chemin1 = "/tmp/FIFO1";
	const char *chemin2 = "/tmp/FIFO2";
		
	at  = malloc(dt.nbrThread * sizeof(ARG_T));
	tid = malloc(dt.nbrThread * sizeof(pthread_t) );
	
	//Creation mutex et barriere
	pthread_mutex_init(&mut, NULL);
	
	mem = initMemoirePhysique(mem, dt.nbrPage, dt.nbrFrames);
	
	creationTube();
	
	//Init les infos pour la structure des threads et les créations des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
	{
		at[cmpt].mut 	  = &mut;
		at[cmpt].hit 	  = 0;
		at[cmpt].numReq   = dt.nbrAcces;
		at[cmpt].nbrPages = dt.nbrPage;
		pthread_create(tid + cmpt, NULL, demandeAcces, at + cmpt);
	}
	
	//Récupération et envoie des données par le thread père
	for(int cmpt = 0; cmpt < dt.nbrThread * dt.nbrAcces; cmpt++)
	{
		adresse = lectureTube(chemin1);
		
		page = LRU(dt.nbrFrames, dt.nbrPage, dt.taillePage, &mem, adresse);
		
		ecritureTube(chemin2, page);
	}
	
	suppressionTube();
	
	//Fin des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
		pthread_join(tid[cmpt], NULL);
	
	//Affichage du nombre de hit par thread
	for(int cmptT = 0; cmptT < dt.nbrThread; cmptT++)
	{
		printf("Nombre de hit pour le thread n°%ld = %d pourcent \n", tid[cmptT], at[cmptT].hit * 100 / dt.nbrAcces);
		moyenneHit += at[cmptT].hit;
	}
	
	//Calcul de la moyenne de hit
	moyenneHit /= dt.nbrThread;
	printf("\nLa moyenne de hit est de : %d pourcent\n", moyenneHit * 100 / dt.nbrAcces);
	
	//Suppresion des mutex et des barrieres 
	pthread_mutex_destroy(&mut);
	
	//Libération de la mémoire alloué
	free(mem.memRapide);
	free(mem.memLente);
	free(mem.tabIndex);
	
	free(at);
	free(tid);
}

int main(int argc, char ** argv)
{
	const char *chemin = "data.cfg";
	
	srand(getpid());
	
	DATA dt;
	
	dt = initStruct(dt, chemin);
	
	afficheConfig(dt);
	
	gestionThread(dt);
	
	return 0;
}
