#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cassert>
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Structure de données contenant le mutex
typedef struct donneeGlobale
{

	//
	// LISTE DES EVENEMENTS POUVANT ETRE ECHANGES ENTRE
	// LE CONTROLEUR DE LA MINE ET LES AUTRES PROCESSUS
	// DE TRAITEMENT
	//

	//
	// LA FILE DE MESSAGE PERMETTANT L'ECHANGE D'INFO
	//

	int pipeControle[2];
	int pipeReservoirEau[2];
	int pipeReservoirFuel[2];
	int pipeCapteurCo2[2];
	int pipeCalculateurMoyenne[2];
	int pipeChaudiereVapeur[2];
	int pipeChaudiereEau[2];

	//
	// LES DIFFERENTS NIVEAUX PREDETERMINES
	//

	const int NIV_EAU_TRES_BAS = 10;
	const int NIV_EAU_BAS = 50;
	const int NIV_EAU_HAUT = 200;
	const int NIV_CO2_TRES_BAS = 1;
	const int NIV_CO2_BAS = 3;
	const int NIV_CO2_HAUT = 5;
	const int NIV_FUEL_TRES_BAS = 100;
	const int NIV_FUEL_BAS = 250;
	const int NIV_FUEL_HAUT = 1000;

	//
	// VARIABLES PARTAGEE CORRESPONDANT AUX E/S DU SYSTEME
	//

	int NIV_ALARME = 1;
	int NIV_EAU = 125;
	int NIV_FUEL = 525;
	int NIV_CO2 = 4;

	int SIM_NIV_EAU = 125;
	int SIM_NIV_FUEL = 525;
	int SIM_NIV_CO2 = 4;

	bool exitCmd = false;
	bool terminerThreads = false;

	bool vanneReservoirEauOuverte = false;
	bool vanneReservoirFuelOuverte = false;
	bool vanneFuelChaudiereVapeurOuverte = false; // Tv
	bool vanneEauChaudiereVapeurOuverte = false;  // Pv
	bool vanneGazChaudiereVapeurOuverte = false;
	bool vanneFuelChaudiereEauOuverte = false;

	bool chaudiereEauMarche = false;
	bool chaudiereVapeurMarche = false;

	bool flammeVapeurAllume = false;
	bool flammeEauAllume = false;

	bool ventilateurVapeurAllume = false;
	bool ventilateurEauAllume = false;
	//
	// LA FILE DE MESSAGE PERMETTANT L'ECHANGE D'INFO
	//

	pthread_t premierThread;
	pthread_t secondThread;
	pthread_t troisiemeThread;
	pthread_t quatriemeThread;
	pthread_t cinquiemeThread;
	pthread_t sixiemeThread;
	pthread_t septiemeThread;
	pthread_t huitiemeThread;
	pthread_t neuviemeThread;
	pthread_t dixiemeThread;
	pthread_t onziemeThread;
	pthread_t douziemeThread;

} donneeGlobale;
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
enum SignalNiveau
{
	EAU_TRES_BAS,
	EAU_BAS,
	EAU_HAUT,
	CO2_BAS,
	CO2_INTERMEDIAIRE,
	CO2_HAUT,
	FUEL_TRES_BAS,
	FUEL_BAS,
	FUEL_HAUT,
};

enum SignalCommande
{
	ACTIVER_GAZ,
	DESACTIVER_GAZ,
	DEMARRER_CHAUDIERE,
	ARRETER_CHAUDIERE,
	FERMER_VANNE_EAU,
	FERMER_VANNE_FUEL,
	ACTIVER_VENTILATION,
	ALLUMER_FLAMME,
};

void *threadTerminal(void *arg)
{
	printf("(II) Starting thread_controleur\n");
	donneeGlobale *md = (donneeGlobale *)arg;

	while (true)
	{
		char ch = getchar();
		switch (ch)
		{
		case 's':
			// start the system
			md->exitCmd = false;
			break;
		case 'S':
			// stop the system
			md->exitCmd = true;
			break;
		case 'i':
		case 'I':
			// printf methane and water level (for debug)
			break;
		case 'f':
		case 'F':
			// close the application
			md->terminerThreads = true;
			break;
		}
	}

	printf("(II) Ending thread_keyboard\n");
	pthread_exit(NULL);
}

void *threadSimulationEau(void *arg)
{

	printf("Startin threadSimulatiionEau\n");
	donneeGlobale *md = (donneeGlobale *)arg;

	while (true)
	{
		if (md->terminerThreads)
		{
			break;
		}
		if (md->vanneReservoirEauOuverte)
		{
			md->SIM_NIV_EAU += 30;
		}
		printf("Niveau d'eau : %d\n", md->SIM_NIV_EAU);
		sleep(2);
	}
}

void *threadSimulationFuel(void *arg)
{

	printf("Startin threadSimulationFuel\n");
	donneeGlobale *md = (donneeGlobale *)arg;

	while (true)
	{
		if (md->terminerThreads)
		{
			break;
		}
		if (md->vanneReservoirFuelOuverte)
		{
			md->SIM_NIV_FUEL += 30;
		}
		printf("Niveau de fuel : %d\n", md->SIM_NIV_FUEL);
		sleep(2);
	}
}

void *threadSimulationCo2(void *arg)
{

	printf("Startin threadSimulationCo2\n");
	donneeGlobale *md = (donneeGlobale *)arg;

	while (true)
	{
		if (md->terminerThreads)
		{
			break;
		}
		if (md->SIM_NIV_CO2 < md->NIV_CO2_HAUT)
		{
			md->SIM_NIV_CO2 += 1;
		}
		else
		{
			md->SIM_NIV_CO2 -= 1;
		}
		printf("Niveau de CO2 : %d\n", md->SIM_NIV_CO2);
		sleep(2);
	}
}

// S'occupe de récupérer les signaux des capteurs d'eau (réservoir d'eau), de fuel (réservoir de fuel) et du capteur de CO2.
// S'occupe d'envoyer les informations de température et de préssion à la chaudière à vapeur sur les 24 heures de la journée (1h = 1 température et 1 pression souhaitée).
// S'occupe d'activer la vanne à gaz pour le brûleur de la chaudière à vapeur lorsque le niveau de fuel est très bas.
//

void *threadControle(void *arg)
{
	printf("Startin threadControle\n");
	donneeGlobale *md = (donneeGlobale *)arg;

	SignalNiveau signalNiveau;
	while (true)
	{
		assert(read(md->pipeControle[0], &signalNiveau, 1) == 1);
		if (md->terminerThreads)
		{
			break;
		}
		switch (signalNiveau)
		{
		case EAU_TRES_BAS:
			// envoyer un signal à l'interface pour allumer une diode rouge du réservoir eua
			// envoyer un signal pour fermer la vanne d'eau de la chaudière à vapeur
			break;
		// // CAS INTERNE AU RESERVOIR
		// case EAU_BAS:
		// 	// envoyer un signal au réservoir d'eau pour ouvrir la vanne d'arrivée d'eau (penser à vérifier si la vanne d'arrivée de fuel du réservoir de fuel n'est pas déjà ouverte)
		// 	break;
		// // CAS INTERNE AU RESERVOIR
		// case EAU_HAUT:
		// 	// envoyer un signal au résrvoir d'eau pour coupper la vanne d'arrivée d'eau
		case FUEL_TRES_BAS:
			// envoyer un signal à l'interface pour allumer une diode rouge du réservoir fuel
			// envoyer signal de basculement à la vanne gaz pour la chaudière à vapeur jusqu'au niveau FUEL_BAS où il y a alors possibilité de rebasculer sur la vanne fuel de la chaudière à  apeur
		// // CAS INTERNE AU RESERVOIR
		// case FUEL_BAS:
		// 	// envoyer un signal au réservoir de fuel pour ouvrir la vanne d'arrivée de fuel (penser à vérifier si la vanne d'arrivée d'eau du réservoir d'eau n'est pas déjà ouverte)
		// // CAS INTERNE AU RESERVOIR
		// case FUEL_HAUT:
		// 	// envoyer un signal au réservoir de fuel pour couper la vanne d'arrivée de fuel
		case CO2_BAS:
			// les deux chaudières peuvent fonctionner, envoyer signal en conséquence
		case CO2_INTERMEDIAIRE:
			// seule la chaudière eau peut fonctionner, envoyer signal en conséquence
		case CO2_HAUT:
			// aucune chaudière ne peut fonctionner, envoyer signal en conséquence
		}
	}
}

// Gestion du réservoir d'eau (capteur d'eau). Ce thread s'occupe d'envoyer les signaux de niveau d'eau très bas
// mais aussi de fermer la vanne d'entrée d'eau si le niveau d'eau est trop haut. Si le niveau d'eau
// est bas, on essaie d'ouvrir la vanne d'eau pour remplir le réservoir.

void *threadReservoirEau(void *arg)
{
	donneeGlobale *md = (donneeGlobale *)arg;
	SignalNiveau signalNiveau;
	while (!md->terminerThreads)
	{
		md->NIV_EAU = md->SIM_NIV_EAU;
		if (md->NIV_EAU < md->NIV_EAU_TRES_BAS)
		{
			signalNiveau = EAU_TRES_BAS;
			assert(write(md->pipeControle[1], &signalNiveau, 1) == 1);
		}
		else if (md->NIV_EAU > md->NIV_EAU_HAUT)
		{
			md->vanneReservoirEauOuverte = false;
		}
		else if (md->NIV_EAU < md->NIV_EAU_BAS)
		{
			if (!md->vanneReservoirFuelOuverte)
			{
				signalNiveau = EAU_BAS;
				md->vanneReservoirEauOuverte = true;
			}
		}
		sleep(2);
	}
	pthread_exit(NULL);
}

// Gestion du réservoir de fuel (capteur de fuel). Ce thread s'occupe d'envoyer les signaux de niveau de fuel très bas
// mais aussi de fermer la vanne d'entrée de fuel si le niveau de fuel est trop haut. Si le niveau de fuel
// est bas, on essaie d'ouvrir la vanne de fuel pour remplir le réservoir.

void *threadReservoirFuel(void *arg)
{
	donneeGlobale *md = (donneeGlobale *)arg;
	SignalNiveau signalNiveau;
	while (!md->terminerThreads)
	{
		md->NIV_FUEL = md->SIM_NIV_FUEL;
		if (md->NIV_FUEL < md->NIV_FUEL_TRES_BAS)
		{
			signalNiveau = FUEL_TRES_BAS;
			assert(write(md->pipeControle[1], &signalNiveau, 1) == 1);
		}
		else if (md->NIV_FUEL > md->NIV_FUEL_HAUT)
		{
			md->vanneReservoirFuelOuverte = false;
		}
		else if (md->NIV_FUEL < md->NIV_FUEL_BAS)
		{
			if (!md->vanneReservoirEauOuverte)
			{
				signalNiveau = FUEL_BAS;
				md->vanneReservoirFuelOuverte = true;
			}
		}
		sleep(2);
	}
	pthread_exit(NULL);
}
// Gestion de capteur de dioxyde de carbone. Il s'occupe d'envoyer les signaux de niveau de Co2 bas et haut.

void *threadCapteurCo2(void *arg)
{
}

// Gestion de la chaudière à vapeur. S'occupe de fermer et ouvrir les vannes internes de fuel,

void *threadChaudiereVapeur(void *arg)
{
	donneeGlobale *md = (donneeGlobale *)arg;
	SignalCommande signalCommande;
	while (!md->terminerThreads)
	{
		assert(read(md->pipeChaudiereVapeur[0], &signalCommande, 1) == 1);
		if (signalCommande == DEMARRER_CHAUDIERE) {
			md->ventilateurVapeurAllume = true;
			sleep(10);
			md->vanneFuelChaudiereVapeurOuverte = true;
			md->flammeVapeurAllume = true; // TODO: A faire évoluer en random sur 3 essai
			if (md->flammeVapeurAllume) {
				md->chaudiereVapeurMarche;
			}
		}

		if (md->chaudiereVapeurMarche)
		{
			if (signalCommande == FERMER_VANNE_FUEL)
			{
				md->vanneFuelChaudiereVapeurOuverte = false;
				
			}
			else if (signalCommande == ARRETER_CHAUDIERE) {
				md->chaudiereVapeurMarche = false;
			}
		}
		sleep(2);
	}
	pthread_exit(NULL);
}

void *threadChaudiereEau(void *arg)
{
}

// Gestion de la moyenne des niveaux du fuel et d'eau dans les réservoirs et de la température de l'eau dans la chaudière eau.

void *threadGestionnaireStatistiques(void *arg)
{
}

// Affichage des informations nécessaires : pression et température pour la chaudière à vapeur, moyenne des niveaux de fuel et d'eau dans les réservoirs, moyenne de la température de l'eau dans la chaudière.

void *threadInterface(void *arg)
{
}

int main()
{

	//
	// Création de la structure de données
	//
	donneeGlobale md;

	//
	// Initialisation des pipe
	//
	int result = pipe(md.pipeCalculateurMoyenne);
	if (result < 0)
	{
		perror("pipe ");
		exit(1);
	}

	result = pipe(md.pipeCapteurCo2);
	if (result < 0)
	{
		perror("pipe ");
		exit(1);
	}

	result = pipe(md.pipeChaudiereEau);
	if (result < 0)
	{
		perror("pipe ");
		exit(1);
	}

	result = pipe(md.pipeChaudiereVapeur);
	if (result < 0)
	{
		perror("pipe ");
		exit(1);
	}

	result = pipe(md.pipeControle);
	if (result < 0)
	{
		perror("pipe ");
		exit(1);
	}

	result = pipe(md.pipeReservoirEau);
	if (result < 0)
	{
		perror("pipe ");
		exit(1);
	}

	result = pipe(md.pipeReservoirFuel);
	if (result < 0)
	{
		perror("pipe ");
		exit(1);
	}
	//
	// Boucle de création des mutex
	//

	// Création des threads
	pthread_create(&(md.premierThread), NULL, threadTerminal, &md);
	pthread_create(&(md.secondThread), NULL, threadControle, &md);
	pthread_create(&(md.troisiemeThread), NULL, threadSimulationEau, &md);
	pthread_create(&(md.quatriemeThread), NULL, threadSimulationFuel, &md);
	pthread_create(&(md.cinquiemeThread), NULL, threadSimulationCo2, &md);
	pthread_create(&(md.sixiemeThread), NULL, threadGestionnaireStatistiques, &md);
	pthread_create(&(md.septiemeThread), NULL, threadInterface, &md);
	pthread_create(&(md.huitiemeThread), NULL, threadReservoirEau, &md);
	pthread_create(&(md.neuviemeThread), NULL, threadReservoirFuel, &md);
	pthread_create(&(md.dixiemeThread), NULL, threadCapteurCo2, &md);
	pthread_create(&(md.onziemeThread), NULL, threadChaudiereVapeur, &md);
	pthread_create(&(md.douziemeThread), NULL, threadChaudiereEau, &md);

	// Attente des threads
	pthread_join(md.premierThread, NULL);
	pthread_join(md.secondThread, NULL);
	pthread_join(md.troisiemeThread, NULL);
	pthread_join(md.quatriemeThread, NULL);
	pthread_join(md.cinquiemeThread, NULL);
	pthread_join(md.sixiemeThread, NULL);
	pthread_join(md.septiemeThread, NULL);
	pthread_join(md.huitiemeThread, NULL);
	pthread_join(md.neuviemeThread, NULL);
	pthread_join(md.dixiemeThread, NULL);
	pthread_join(md.onziemeThread, NULL);
	pthread_join(md.douziemeThread, NULL);

	//
	// Destruction des mutex
	//

	return EXIT_SUCCESS;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//