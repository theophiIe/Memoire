#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

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
	pthread_mutex_t   *mut;
	pthread_barrier_t *bar;
	pthread_t tid;
	
} arg_t;

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

void afficheConfig(DATA dt)
{
	printf("%d\n", dt.nbrFrames);
	printf("%d\n", dt.taillePage);
	printf("%d\n", dt.nbrPage);
	printf("%d\n", dt.nbrThread);
	printf("%d\n", dt.nbrAcces);
}

void * sommeTableau (void * arg)
{
	arg_t *at = (arg_t *) arg;
	

	
	return NULL;
}

void progPrincipal(DATA dt)
{
	pthread_mutex_t   mut;
	pthread_barrier_t bar;
	
	arg_t *at;
	
	at = malloc(dt.nbrThread * sizeof(arg_t));
	
	pthread_mutex_init(&mut, NULL);
	pthread_barrier_init(&bar, NULL, dt.nbrThread);
	
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
	{
		at[cmpt].mut = &mut;
		at[cmpt].bar = &bar;
	}
	
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
		pthread_create(&at[cmpt].tid, NULL, sommeTableau, &at[cmpt]);
	
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
		pthread_join(at[cmpt].tid, NULL);
	
	pthread_mutex_destroy(&mut);
	pthread_barrier_destroy(&bar);	
	
	free(at);
}

int main()
{
	const char *chemin = "data.cfg";
	
	pthread_mutex_t   mut;
	pthread_barrier_t bar;
	
	DATA dt = initStruct(chemin);
	
	afficheConfig(dt);
	
	progPrincipal(dt);
	
	return 0;
}
