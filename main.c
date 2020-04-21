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

DATA initStruct(const char *chemin)
{
	int num;
	int rd;
	
	DATA dt;
	SE_FICHIER file;
	
	file = SE_ouverture (chemin, O_RDONLY);
	
	if (file.descripteur < 0) 
	{
		fprintf(stderr, "ERR: opening cfg file\n");
		exit(EXIT_FAILURE);
	}

	rd = SE_lectureEntier(file, &num);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrFrames\n");
		goto err_rd;
	}
	
	dt.nbrFrames = num;

	rd = SE_lectureEntier(file, &num);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing taillePage\n");
		goto err_rd;
	}
	
	dt.taillePage = num;
	
	rd = SE_lectureEntier(file, &num);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrPage\n");
		goto err_rd;
	}
	
	dt.nbrPage = num;
	
	rd = SE_lectureEntier(file, &num);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrThread\n");
		goto err_rd;
	}
	
	dt.nbrThread = num;
	
	rd = SE_lectureEntier(file, &num);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrAcces\n");
		goto err_rd;
	}
	
	dt.nbrAcces = num;
	
	SE_fermeture(file);
	
	return dt;
	
	err_rd:
	SE_fermeture(file);
	exit(EXIT_FAILURE);
}

void afficheConfig(DATA dt)
{
	printf("%d\n", dt.nbrFrames);
	printf("%d\n", dt.taillePage);
	printf("%d\n", dt.nbrPage);
	printf("%d\n", dt.nbrThread);
	printf("%d\n", dt.nbrAcces);
}

int main()
{
	const char *chemin = "data.cfg";

	DATA dt = initStruct(chemin);
	
	afficheConfig(dt);
	
	return 0;
}
