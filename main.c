#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

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
	int *tabIndexe;		//< Indexe pour le LRU
	
} MEMORY;

typedef struct 
{
	pthread_mutex_t *mut;
	
	int hit;		//< Nombre de hit
	int numReq;		//< Nombre de requête
	
} ARG_T;

DATA initStruct(const char *chemin)
{
	int rd;
	
	DATA dt;
	SE_FICHIER file;
	
	file = SE_ouverture (chemin, O_RDONLY);
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

MEMORY initMemoirePhysique(int nbrPage, int nbrFrames)
{
	MEMORY mem;
	
	mem.memLente = malloc(nbrPage * sizeof(int));
	for(int size = 0; size < nbrPage; size++)
		mem.memLente[size] = -1;
	
	mem.memRapide = malloc(nbrFrames * sizeof(int));
	for(int size = 0; size < nbrFrames; size++)
		mem.memRapide[size] = -1;
	
	mem.tabIndexe = malloc(nbrFrames * sizeof(int));
	for(int size = 0; size < nbrFrames; size++)
		mem.tabIndexe[size] = -1;
		
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


int LRU(int nbrFrame, int nbrPage, MEMORY *mem, int adresse)
{
	int valMax = 0;
	
	//Recheche dans la mémoire Rapide
	for(int cmptR = 0; cmptR < nbrFrame; cmptR++)
	{
		if(mem -> memRapide[cmptR] == -1)
		{
			mem -> memRapide[cmptR] = adresse;
			
			for(int cmptI = 0; cmptI < nbrFrame; cmptI++)
			{
				if(mem -> tabIndexe[cmptI] != -1 && cmptI != cmptR)
					mem -> tabIndexe[cmptI] += 1;
				
				else if(cmptI == cmptR)
					mem -> tabIndexe[cmptI] = 0;
			}
			
			return 0; //Voir quoi return si rien d'allouer ?
		}
		
		else if(mem -> memRapide[cmptR] == adresse)
		{
			for(int cmptI = 0; cmptI < nbrFrame; cmptI++)
			{
				if(mem -> tabIndexe[cmptI] != -1 && cmptI != cmptR)
					mem -> tabIndexe[cmptI] += 1;
				
				else if(cmptI == cmptR)
					mem -> tabIndexe[cmptI] = 0;
			}
			
			return mem -> memRapide[cmptR]; //Pas sur ?
		}
	}
	
	//Recherche dans la memoire lente
	for(int cmptL = 0; cmptL < nbrPage; cmptL++)
	{
		if(mem -> memLente[cmptL] == adresse)
		{
			//Recherche de la postion du plus ancien
			for(int cmptI = 0; cmptI < nbrFrame; cmptI++)
			{
				if(mem -> tabIndexe[cmptI] > valMax)
					valMax = mem -> tabIndexe[cmptI];
			}
			
			//Modification du cache interversion de la valeur memL avec memR
			for(int cmptR = 0; cmptR < nbrFrame; cmptR++)
			{
				if(mem -> tabIndexe[cmptR] == valMax)
				{
					mem -> memLente[cmptL] = mem -> memRapide[cmptR];
					mem -> memRapide[cmptR] = adresse;
					
					//Mise à jour de l'indexe
					for(int cmptI = 0; cmptI < nbrFrame; cmptI++)
					{
						if(mem -> tabIndexe[cmptI] != -1 && cmptI != cmptR)
							mem -> tabIndexe[cmptI] += 1;
						
						else if(cmptI == cmptR)
							mem -> tabIndexe[cmptI] = 0;
					}
					
					return -1;// Trouver quoi return si le thread va cherhcer l'info dans la memoire lente
				}	
			}
		}
	}
	
	return -1; //Si on arrive ici on a une erreur !
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
	
	valFIFO = mkfifo("/tmp/FIFO2", 0666);
	if(valFIFO != 0)
	{
		fprintf(stderr, "\nERR: creating fifo2\n");
		exit(EXIT_FAILURE);
	}
}

void suppressionTube()
{
	unlink("/tmp/FIFO1");
	unlink("/tmp/FIFO2");
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
	
	SE_fermeture (tube);
	
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
		
	printf("debut du thread fils : %ld\n", pthread_self());
	for(int cmpt = 0; cmpt < at -> numReq; cmpt++)
	{
		//Faire l'appel à la mutex
		pthread_mutex_lock(at -> mut);
		
		printf("tid thread fils : %ld\n", pthread_self());
		
		ecritureTube(chemin1, cmpt);
		
		reponse = lectureTube(chemin2);
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
	
	mem = initMemoirePhysique(dt.nbrPage, dt.nbrFrames);
	
	//~ //Init des infos pour la structure des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
	{
		at[cmpt].mut = &mut;
		at[cmpt].hit = 0;
		at[cmpt].numReq = dt.nbrAcces;
		pthread_create(tid + cmpt, NULL, demandeAcces, at + cmpt);
	}
	
	printf("Début du thread père\n");
	printf("for = %d\n", dt.nbrThread * dt.nbrAcces);
	
	creationTube();
	
	for(int cmpt = 0; cmpt < dt.nbrThread * dt.nbrAcces; cmpt++)
	{
		adresse = lectureTube(chemin1);
		
		page = LRU(dt.nbrFrames, dt.nbrPage, &mem, adresse);
		
		ecritureTube(chemin2, page);
	}
	
	suppressionTube();
	
	//Fin des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
		pthread_join(tid[cmpt], NULL);
	
	for(int cmptT = 0; cmptT < dt.nbrThread; cmptT++)
	{
		printf("Nombre de hit pour le thread n° = %d\n", at[cmptT].hit);
		moyenneHit += at[cmptT].hit;
	}
	
	moyenneHit /= dt.nbrThread;
	printf("La moyenne de hit est de : %d\n", moyenneHit);
	
	//Suppresion des mutex et des barrieres 
	pthread_mutex_destroy(&mut);
	
	free(at);
	free(tid);
}

int main(int argc, char ** argv)
{
	const char *chemin = "data.cfg";
	
	DATA dt = initStruct(chemin);
	
	afficheConfig(dt);
	
	gestionThread(dt);
	
	return 0;
}
