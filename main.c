#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
	
} MEMORY;

typedef struct 
{
	pthread_mutex_t *mut;
	
	int hit;		//< Nombre de hit
	int numReq;		//< Nombre de requête
	int start;
	
} ARG_T;

DATA initStruct(const char *chemin)
{
	int rd;
	
	DATA dt;
	SE_FICHIER file;
	
	file = SE_ouverture (chemin, O_RDONLY);
	if (file.descripteur < 0) 
	{
		fprintf(stderr, "ERR: opening cfg file\n");
		exit(EXIT_FAILURE);
	}

	rd = SE_lectureEntier(file, &dt.nbrFrames);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrFrames\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}

	rd = SE_lectureEntier(file, &dt.taillePage);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing taillePage\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrPage);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrPage\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrThread);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrThread\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrAcces);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrAcces\n");
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

int *LRU(int frame)
{
	int *cache;
	
	return cache;
}

void * demandeAcces (void * arg)
{
	ARG_T *at = (ARG_T *) arg;
	
	int valRD, valWR, nb_numbers;
	
	//Création du chemin pour le tube
	const char *chemin1 = "/tmp/FIFO1";
	const char *chemin2 = "/tmp/FIFO2";
			
	printf("debut du thread fils\n");
	for(int cmpt = at -> start; cmpt < at -> numReq; cmpt++)
	{
		//Faire l'appel à la mutex
		pthread_mutex_lock(at -> mut);
		
		printf("debut du thread fils entrant dans le mutex\n");
		
		//Ouverture du tube en mode ecriture
		printf("Tentative d'ouverture du tube n°1 par le fils\n");
		SE_FICHIER tube = SE_ouverture(chemin1, O_WRONLY);
		if (tube.descripteur < 0) 
		{
			fprintf(stderr, "ERR: opening pipe\n");
			exit(EXIT_FAILURE);
		}
		printf("Ouverture réussite du tube n°1 par le fils\n");
		
		//Ectiture de la demande dans un tube en mode ecriture
		sleep(0.1);
		printf("tid : %ld\n", pthread_self());
		printf("Tentative d'ecriture pipe n°1 fils\n");
		valWR = SE_ecritureEntier(tube, pthread_self());
		if (valWR <= 0) 
		{
			fprintf(stderr, "ERR: write in pipe\n");
			SE_fermeture(tube);
			exit(EXIT_FAILURE);
		}
		
		if(SE_ecritureCaractere (tube, ' ') == -1)
		{
			printf("Une erreur d'écriture a eu lieu\n");
			return -1;
		}
		printf("Ecriture reussite pipe n°1 fils\n");
		
		//Fermeture du tube
		SE_fermeture (tube);
		printf("Fermeture reussite pipe n°1 fils\n");
		
		sleep(0.2);
		
		//Ouverture du tube pour la lecture
		printf("Tentative d'ouveture du pipe n°2 fils\n");
		tube = SE_ouverture(chemin2, O_RDONLY);
		if (tube.descripteur < 0) 
		{
			fprintf(stderr, "ERR: opening pipe\n");
			exit(EXIT_FAILURE);
		}
		printf("Ouverture réussite du tube n°2 par le fils\n");
		
		//Lecture de la reponse de la part du pere
		sleep(0.1);
		printf("Tentative de lecture pipe n°2 fils\n");
		valRD = SE_lectureEntier(tube, &nb_numbers);
		if (valRD <= 0) 
		{
			fprintf(stderr, "ERR: read in pipe\n");
			SE_fermeture(tube);
			exit(EXIT_FAILURE);
		}
		
		printf("Lecture reussite pipe n°2 fils : %d\n", nb_numbers);
		
		//Fermeture et Suppression du tube
		SE_fermeture (tube);
		printf("Fermeture du tube n°2 fils\n");
		
		unlink(chemin2);
		printf("Supression du tube n°2 fils\n");
		
		//Fin du mutex
		pthread_mutex_unlock(at -> mut);
		printf("Fin du mutex\n");
	}
	
	return NULL;
}

void gestionThread(DATA dt)
{
	pthread_mutex_t mut;
	pthread_t * tid;
	
	ARG_T *at;
	
	const char *chemin1 = "/tmp/FIFO1";
	const char *chemin2 = "/tmp/FIFO2";
	int valWR, valRD, nb_numbers, valFIFO;
	
	at  = malloc(dt.nbrThread * sizeof(ARG_T));
	tid = malloc (dt.nbrThread * sizeof (pthread_t) );
	
	//Creation mutex et barriere
	pthread_mutex_init(&mut, NULL);
	
	//~ //Init des infos pour la structure des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
	{
		at[cmpt].mut = &mut;
		at[cmpt].hit = 0;
		at[cmpt].numReq = dt.nbrAcces;
		at[cmpt].start = 0;
		pthread_create(tid + cmpt, NULL, demandeAcces, at + cmpt);
	}
	
	//Communication par tubes

	printf("Début du thread père\n");
	printf("for = %d\n", dt.nbrThread * dt.nbrAcces);
	for(int cmpt = 0; cmpt < dt.nbrThread * dt.nbrAcces; cmpt++)
	{
		//Probleme comment connaitre le tid de lecture var global?
		//Recupération du chemin
		printf("Creation du thread n°1\n");
		valFIFO = mkfifo(chemin1, 0600);
		if (valFIFO) 
		{
			fprintf(stderr, "ERR: creating pipe n°1\n");
			unlink(chemin1);
			exit(EXIT_FAILURE);
		}
		
		//Ouverture du tube
		sleep(0.2);
		printf("Tentative d'ouverture du tube n°1 par le père\n");
		SE_FICHIER tube = SE_ouverture(chemin1, O_RDONLY);
		if (tube.descripteur < 0) 
		{
			fprintf(stderr, "ERR: opening pipe\n");
			exit(EXIT_FAILURE);
		}
		
		printf("Ouverture réussite du tube n°1 par le père\n");
		
		//Lecture du tube avec comme chemin le tid du thread
		printf("Tentative de lecture du tube n°1 par le pere\n");
		valRD = SE_lectureEntier(tube, &nb_numbers);
		if (valRD <= 0) 
		{
			fprintf(stderr, "ERR: missing nb_numbers\n");
			SE_fermeture(tube);
			exit(EXIT_FAILURE);
		}
		
		printf("Lecture réussite du pipe père n°1 : %d\n", nb_numbers);
		
		//Recuperation de l'information ecrite dans le tube 
		//Fermeture et Suppresion du tube 
		SE_fermeture (tube);
		printf("Fermuture du tube n°1 par le pere\n");
		
		unlink(chemin1);
		printf("Suppression du tube n°1 par le pere\n");
		
		//Utiliser l'algo de LRU pout verifier la demande soit presente dans le cache
		//Creation du tube à partir du même tid que le tube du mode lecture
		printf("Creation du pipe père n°2\n");
		valFIFO = mkfifo(chemin2, 0600);
		if (valFIFO) 
		{
			fprintf(stderr, "ERR: creating pipe n°2\n");
			unlink(chemin2);
			exit(EXIT_FAILURE);
		}
		
		//Si la valeur est presente renvoyer la valeur grace au tube en ecriture
		//Ouverture du tube en mode ecriture
		printf("Tentative d'ouverture du pipe père n°2\n");
		tube = SE_ouverture(chemin2, O_WRONLY);
		if (tube.descripteur < 0) 
		{
			fprintf(stderr, "ERR: opening pipe\n");
			exit(EXIT_FAILURE);
		}
		
		printf("Ouverture reussite du pipe père n°2\n");
		
		//Ecriture dans le tube
		sleep(0.1);
		printf("Tentative d'ecriture pipe n°2 père\n");
		valWR = SE_ecritureEntier(tube, pthread_self());
		if (valWR <= 0) 
		{
			fprintf(stderr, "ERR: write in pipe\n");
			SE_fermeture(tube);
			exit(EXIT_FAILURE);
		}
		
		if(SE_ecritureCaractere (tube, ' ') == -1)
		{
			printf("Une erreur d'écriture a eu lieu\n");
			return -1;
		}
		
		printf("Ecriture reussite pipe n°2 père\n");
		//Sinon renvoyé 0 par exemple 
		
		//Fermeture du tube
		SE_fermeture (tube);
		printf("Fermeture du pipe n°2 père\n");
	}
	
	//Fin des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
		pthread_join(tid[cmpt], NULL);
	
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
