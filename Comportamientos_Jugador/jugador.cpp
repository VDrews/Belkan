#include "../Comportamientos_Jugador/jugador.hpp"
#include "motorlib/util.h"
#include <chrono>
#include "./heap.cpp"
#include <omp.h>


#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>
#include <stack>
#include <cstdlib>

using namespace std::chrono;

// Indica si se ha recargado, para no hacer la recarga varias veces consecutivas
bool recargando = false;

// Indica si se ha cogido el bikini ya
bool has_bikini = false;

// Indica si se ha cogido las zapatillas ya
bool has_zapatillas = false;

// Si se encuentra una bateria en el mapa, lo guardaremos aqui
estado bateria;
bool bateriaEncontrada = false;


// Calcula la distancia entre dos nodos
int distancia(Nodo a, Nodo b, int nivel) {
	int distX = abs(a.st.columna - b.st.columna);
	int distY = abs(a.st.fila - b.st.fila);

	// Lo multiplicamos por el coste medio
	return (distX + distY) * 40;
}

// Comprueba en paralelo si existe ese nodo en el set
bool contains(Nodo nodo, set<Nodo> &nodos) {
	#pragma omp parallel
	{
		#pragma omp for nowait
		for(set<Nodo>::iterator itv = nodos.begin(); itv != nodos.end(); ++itv) {
			if (*itv == nodo) {
				return true;
			}
		}
		return false;
	}
}

// Dada la orientacion del jugador, y la dirección en la que se encuentra la casilla (0 -> norte, 1 -> este...)
// Indica el giro que tendría que hacer para apuntar a esa casilla 0: Recto, 1: Derecha, -1: Izquierda, 2: Da la vuelta
int girar(int orientacion, int direccionDestino) {
	if (orientacion == direccionDestino) {
		return 0;
	}

	else if ((orientacion+1)%4 == direccionDestino) {
		return 1;
	}

	else if ((orientacion-1)%4 == direccionDestino) {
		return -1;
	}

	else if ((orientacion+2)%4 == direccionDestino) {
		return 2;
	}

	else if (orientacion == 0 && direccionDestino == 3) {
		return -1;
	}

	else if (orientacion == 3 && direccionDestino == 0) {
		return 1;
	}

	else {
		cout << "Error de giro: " << orientacion << "/" << direccionDestino << endl;
		throw 33;
	}
}

// Añade al plan los actIDLE necesarios para recargarse al completo
void recarga(list<Action> &plan, Sensores sensores) {
	cout << "RECARGANDO: " << sensores.bateria << endl;
	for(int batActual = sensores.bateria; batActual < 3000; batActual += 10) {
		plan.push_front(actIDLE);
	}
	cout << endl;
}

// Este es el método principal que debe contener los 4 Comportamientos_Jugador
// que se piden en la práctica. Tiene como entrada la información de los
// sensores y devuelve la acción a realizar.
Action ComportamientoJugador::think(Sensores sensores) {
	Action Action = actIDLE;

	actual.fila        = sensores.posF;
	actual.columna     = sensores.posC;
	actual.orientacion = sensores.sentido;

	// Cambia de destino a la bateria si la bateria es baja y conoce alguna cercana
	if (sensores.bateria <= 500 && bateriaEncontrada) {
		destino.fila       = bateria.fila;
		destino.columna    = bateria.columna;
	}
	else {
		destino.fila       = sensores.destinoF;
		destino.columna    = sensores.destinoC;
	}

	// Si llega al destino, se espera a que cambie el objetivo
	if (destino.columna == actual.columna && destino.fila == actual.fila) {
		return actIDLE;
	}

	// Si el plan está vacío, genera un plan
	if (plan.empty()) {
		if (sensores.nivel != 4){
			bool hay_plan = pathFinding(sensores.nivel, actual, destino, plan, sensores);
		}
		else {
			// Estoy en el nivel 2
			bool hay_plan = pathFinding(3, actual, destino, plan, sensores);
		}
	}

	// Si hay un aldeano en frente, se espera a que se quite
	else if (sensores.superficie[2] == 'a') return actIDLE;

	// Si hay un obstaculo delante y el jugador quiere seguir hacia delante
	// O si va a entrar en una casilla de agua o de bosque
	// Recalcula el plan
	// Esto no se hará si la casilla en la que se encuentra ahora mismo es una bateria
	else if (((HayObstaculoDelante(actual) && plan.front() == actFORWARD) || 
	(sensores.terreno[0] != 'A' && sensores.terreno[2] == 'A' && plan.front() == actFORWARD) || 
	(sensores.terreno[0] != 'B' && sensores.terreno[2] == 'B' && plan.front() == actFORWARD)) && sensores.terreno[0] != 'X') {
		if (sensores.nivel != 4){
			bool hay_plan = pathFinding(sensores.nivel, actual, destino, plan, sensores);
		}
		else {
			// Estoy en el nivel 2
			bool hay_plan = pathFinding(3, actual, destino, plan, sensores);
		}
	}

	// Si estamos en nivel 4, pintamos en el mapa las casillas que se van viendo
	if (sensores.nivel == 4) {
		mapear(sensores);
	}

	// Si está encima de la bateria, se pone a recargar
	if (sensores.terreno[0] == 'X' && !recargando) {
		recargando = true;
		recarga(plan, sensores);
	}

	// Cuando salga de la bateria, se pone recargando otra vez a false
	if (sensores.terreno[0] != 'X') recargando = false;

	// Actualizamos bikini y zapatillas si estamos encima de alguna
	if (sensores.terreno[0] == 'K') has_bikini = true;
	if (sensores.terreno[0] == 'D') has_zapatillas = true;

  Actiones++;

  Action = plan.front();
  plan.pop_front();
  return Action;
}

// Busca en los sensores la bateria y pinta en el mapa lo que va viendo
void ComportamientoJugador::mapear(Sensores sensores) {

	switch(sensores.sentido) {
		case 0:
			if (!bateriaEncontrada) {
				for (int i  = 0; i<4; i++){
					for(int j = -3; j<4; j++){
						if(mapaResultado[sensores.posF-i][sensores.posC+j] == 'X' && !bateriaEncontrada){
							bateria.columna =sensores.posC+j;
							bateria.fila = sensores.posF-i;
							bateriaEncontrada=true;
						}
					}
				}
			}
			mapaResultado[sensores.posF][sensores.posC] = sensores.terreno[0];
			mapaResultado[sensores.posF-1][sensores.posC-1] = sensores.terreno[1];
			mapaResultado[sensores.posF-1][sensores.posC] = sensores.terreno[2];
			mapaResultado[sensores.posF-1][sensores.posC+1] = sensores.terreno[3];
			mapaResultado[sensores.posF-2][sensores.posC-2] = sensores.terreno[4];
			mapaResultado[sensores.posF-2][sensores.posC-1] = sensores.terreno[5];
			mapaResultado[sensores.posF-2][sensores.posC] = sensores.terreno[6];
			mapaResultado[sensores.posF-2][sensores.posC+1] = sensores.terreno[7];
			mapaResultado[sensores.posF-2][sensores.posC+2] = sensores.terreno[8];
			mapaResultado[sensores.posF-3][sensores.posC-3] = sensores.terreno[9];
			mapaResultado[sensores.posF-3][sensores.posC-2] = sensores.terreno[10];
			mapaResultado[sensores.posF-3][sensores.posC-1] = sensores.terreno[11];
			mapaResultado[sensores.posF-3][sensores.posC] = sensores.terreno[12];
			mapaResultado[sensores.posF-3][sensores.posC+1] = sensores.terreno[13];
			mapaResultado[sensores.posF-3][sensores.posC+2] = sensores.terreno[14];
			mapaResultado[sensores.posF-3][sensores.posC+3] = sensores.terreno[15];
			break;
		case 1:
			if (!bateriaEncontrada) {
				for (int i  = -3; i<4; i++){
					for(int j = 0; j<4; j++){
						if(mapaResultado[sensores.posF-i][sensores.posC+j] == 'X' && !bateriaEncontrada){
							bateria.columna =sensores.posC+j;
							bateria.fila = sensores.posF-i;
							bateriaEncontrada=true;
						}
					}
				}
			}
			mapaResultado[sensores.posF][sensores.posC] = sensores.terreno[0];
			mapaResultado[sensores.posF-1][sensores.posC+1] = sensores.terreno[1];
			mapaResultado[sensores.posF][sensores.posC+1] = sensores.terreno[2];
			mapaResultado[sensores.posF+1][sensores.posC+1] = sensores.terreno[3];
			mapaResultado[sensores.posF-2][sensores.posC+2] = sensores.terreno[4];
			mapaResultado[sensores.posF-1][sensores.posC+2] = sensores.terreno[5];
			mapaResultado[sensores.posF][sensores.posC+2] = sensores.terreno[6];
			mapaResultado[sensores.posF+1][sensores.posC+2] = sensores.terreno[7];
			mapaResultado[sensores.posF+2][sensores.posC+2] = sensores.terreno[8];
			mapaResultado[sensores.posF-3][sensores.posC+3] = sensores.terreno[9];
			mapaResultado[sensores.posF-2][sensores.posC+3] = sensores.terreno[10];
			mapaResultado[sensores.posF-1][sensores.posC+3] = sensores.terreno[11];
			mapaResultado[sensores.posF][sensores.posC+3] = sensores.terreno[12];
			mapaResultado[sensores.posF+1][sensores.posC+3] = sensores.terreno[13];
			mapaResultado[sensores.posF+2][sensores.posC+3] = sensores.terreno[14];
			mapaResultado[sensores.posF+3][sensores.posC+3] = sensores.terreno[15];
			break;
		case 2:
			if (!bateriaEncontrada) {
				for (int i  = 0; i<4; i++){
					for(int j = -3; j<4; j++){
						if(mapaResultado[sensores.posF-i][sensores.posC+j] == 'X' && !bateriaEncontrada){
							bateria.columna =sensores.posC+j;
							bateria.fila = sensores.posF-i;
							bateriaEncontrada=true;
						}
					}
				}
			}
			mapaResultado[sensores.posF][sensores.posC] = sensores.terreno[0];
			mapaResultado[sensores.posF+1][sensores.posC+1] = sensores.terreno[1];
			mapaResultado[sensores.posF+1][sensores.posC] = sensores.terreno[2];
			mapaResultado[sensores.posF+1][sensores.posC-1] = sensores.terreno[3];
			mapaResultado[sensores.posF+2][sensores.posC+2] = sensores.terreno[4];
			mapaResultado[sensores.posF+2][sensores.posC+1] = sensores.terreno[5];
			mapaResultado[sensores.posF+2][sensores.posC] = sensores.terreno[6];
			mapaResultado[sensores.posF+2][sensores.posC-1] = sensores.terreno[7];
			mapaResultado[sensores.posF+2][sensores.posC-2] = sensores.terreno[8];
			mapaResultado[sensores.posF+3][sensores.posC+3] = sensores.terreno[9];
			mapaResultado[sensores.posF+3][sensores.posC+2] = sensores.terreno[10];
			mapaResultado[sensores.posF+3][sensores.posC+1] = sensores.terreno[11];
			mapaResultado[sensores.posF+3][sensores.posC] = sensores.terreno[12];
			mapaResultado[sensores.posF+3][sensores.posC-1] = sensores.terreno[13];
			mapaResultado[sensores.posF+3][sensores.posC-2] = sensores.terreno[14];
			mapaResultado[sensores.posF+3][sensores.posC-3] = sensores.terreno[15];
			break;
		case 3:
			if (!bateriaEncontrada) {
				for (int i  = -3; i<4; i++){
					for(int j = -3; j<1; j++){
						if(mapaResultado[sensores.posF-i][sensores.posC+j] == 'X' && !bateriaEncontrada){
							bateria.columna =sensores.posC+j;
							bateria.fila = sensores.posF-i;
							bateriaEncontrada=true;
						}
					}
				}
			}
			mapaResultado[sensores.posF][sensores.posC] = sensores.terreno[0];
			mapaResultado[sensores.posF+1][sensores.posC-1] = sensores.terreno[1];
			mapaResultado[sensores.posF][sensores.posC-1] = sensores.terreno[2];
			mapaResultado[sensores.posF-1][sensores.posC-1] = sensores.terreno[3];
			mapaResultado[sensores.posF+2][sensores.posC-2] = sensores.terreno[4];
			mapaResultado[sensores.posF+1][sensores.posC-2] = sensores.terreno[5];
			mapaResultado[sensores.posF][sensores.posC-2] = sensores.terreno[6];
			mapaResultado[sensores.posF-1][sensores.posC-2] = sensores.terreno[7];
			mapaResultado[sensores.posF-2][sensores.posC-2] = sensores.terreno[8];
			mapaResultado[sensores.posF+3][sensores.posC-3] = sensores.terreno[9];
			mapaResultado[sensores.posF+2][sensores.posC-3] = sensores.terreno[10];
			mapaResultado[sensores.posF+1][sensores.posC-3] = sensores.terreno[11];
			mapaResultado[sensores.posF][sensores.posC-3] = sensores.terreno[12];
			mapaResultado[sensores.posF-1][sensores.posC-3] = sensores.terreno[13];
			mapaResultado[sensores.posF-2][sensores.posC-3] = sensores.terreno[14];
			mapaResultado[sensores.posF-3][sensores.posC-3] = sensores.terreno[15];
			break;

	}
}


// Llama al algoritmo de busqueda que se usará en cada comportamiento del agente
// Level representa el comportamiento en el que fue iniciado el agente.
bool ComportamientoJugador::pathFinding (int level, const estado &origen, const estado &destino, list<Action> &plan, Sensores sensores){
	switch (level){
		case 1: cout << "Busqueda en profundad\n";
			      return pathFinding_Profundidad(origen,destino,plan);
						break;
		case 2: cout << "Busqueda en Anchura\n";
				//   return pathFinding_Anchura(origen, destino, plan);
				return encontrarCamino(origen,destino,plan, false, sensores);
						break;
		case 3: cout << "Busqueda Costo Uniforme\n";
						// Incluir aqui la llamada al busqueda de costo uniforme
				return encontrarCamino(origen,destino,plan, true, sensores);
						break;
		case 4: cout << "Busqueda para el reto\n";

						// Incluir aqui la llamada al algoritmo de búsqueda usado en el nivel 2
						break;
	}
	cout << "Comportamiento sin implementar\n";
	return false;
}


// Se llamará para encontrar el plan tanto en C.Uniforme como anchura
bool ComportamientoJugador::encontrarCamino(const estado &origen, const estado &destino, list<Action> &plan, bool conBateria, Sensores sensores) {
	auto start = high_resolution_clock::now();
	
	// Limpiamos el plan
	plan.clear();

	Nodo *nodoOrigen = new Nodo(origen, mapaResultado, conBateria);
	Nodo *nodoDestino = new Nodo(destino, mapaResultado, conBateria);

	Heap<Nodo> open;
	set<Nodo> closed;

	// Insertamos el nodo de origen en la lista de abiertos
	open.insert(*nodoOrigen);

	while (!open.empty()) {
		// Obtenemos el nodo con menor coste y lo quitamos de la lista de abiertos
		Nodo current = open.remove();
		list<Nodo>::iterator it;

		// Lo añadimos en la lista de cerrados
		closed.insert(current);

		current.debug();

		list <Nodo> vecinos;

		GetVecinos(&current, vecinos, conBateria);

		// Para cada vecino:
		for(it = vecinos.begin(); it != vecinos.end(); ++it) {
			
			// Calculamos el coste del terreno
			int coste = it->costeTerreno(sensores, has_bikini, has_zapatillas);
			
			// Descartar si es 'P' o 'M', aunque ya se descarta al obtener los vecinos
			if (coste == NULL) continue;
			
			// Si no está en la lista de cerrados
			bool found = (open.contains(*it));
			if(!contains(*it, closed) && !found) {

				// Calculamos gCost y hCost
				int costeDeMoverseAlVecino;
				if (it->giro == 0) {
					costeDeMoverseAlVecino = current.gCost + coste;
				}
				else if (it->giro == 1 || it->giro == -1) {
					costeDeMoverseAlVecino = current.gCost + 2* (coste);
				}
				else if (it->giro == 2) {
					costeDeMoverseAlVecino = current.gCost + 3* (coste);
				}

				it->gCost = costeDeMoverseAlVecino;
				it->hCost = distancia(*it, *nodoDestino, sensores.nivel);
				it->insertParent(new Nodo(current));

				if (!found) {
					open.insert(*it);

					if (*it == *nodoDestino) {
						auto stop = high_resolution_clock::now();
						auto duration = duration_cast<microseconds>(stop - start);
						cout << "T. Ejecucion: " << duration.count() << endl;

						cout << "HA EMPEZADO A TRAZAR EL PLAN" << endl;
						RetrazarPlan(*nodoOrigen, *it, plan);
						cout << "Longitud del plan: " << plan.size() << endl;
						PintaPlan(plan);
						// ver el plan en el mapa
						VisualizaPlan(origen, plan);
						return true;
					}
					// else if (closed.size() >= 800) {

					// Troceo del plan para no tener que planificar 
					// 	auto stop = high_resolution_clock::now();
					// 	auto duration = duration_cast<microseconds>(stop - start);
					// 	cout << "T. Ejecucion: " << duration.count() << endl;

					// 	cout << "HA EMPEZADO A TRAZAR EL PLAN SIMPLIFICADO" << endl;
					// 	RetrazarPlan(*nodoOrigen, open.nearest(), plan);
					// 	cout << "Longitud del plan: " << plan.size() << endl;
					// 	PintaPlan(plan);
					// 	// ver el plan en el mapa
					// 	VisualizaPlan(origen, plan);
					// 	return true;

					// }
				}
				
			}
		}


	}

	cout << "Se ha vaciado la lista de candidatos..." << endl;
	return false;

}

// Dado un destino y un origen, traza todo el trayecto recorriendo todos los padres de los nodos
void ComportamientoJugador::RetrazarPlan(Nodo origen, Nodo destino, list<Action> &plan) {
	Nodo *current = &destino;
	while (*(current) != origen) {

		current->debug();

		giroToAction(current->giro, plan);

		current = current->parent;
	}

	plan.reverse();
	plan.push_back(actFORWARD);
	plan.pop_front();
}

void ComportamientoJugador::GetVecinos(Nodo *nodo, list<Nodo> &vecinos, bool conBateria) {

	estado arriba;
	arriba.columna = nodo->st.columna;
	arriba.fila = nodo->st.fila-1;
	arriba.orientacion = 0;

	estado abajo;
	abajo.columna = nodo->st.columna;
	abajo.fila = nodo->st.fila+1;
	abajo.orientacion = 2;

	estado derecha;
	derecha.columna = nodo->st.columna+1;
	derecha.fila = nodo->st.fila;
	derecha.orientacion = 1;

	estado izquierda;
	izquierda.columna = nodo->st.columna-1;
	izquierda.fila = nodo->st.fila;
	izquierda.orientacion = 3;

	Nodo *nodoArriba = new Nodo(arriba, mapaResultado, conBateria);
	// nodoArriba.insertParent(*nodo);
	Nodo *nodoAbajo = new Nodo(abajo, mapaResultado, conBateria);
	// nodoAbajo.insertParent(*nodo);
	Nodo *nodoIzquierda = new Nodo(izquierda, mapaResultado, conBateria);
	// nodoIzquierda.insertParent(*nodo);
	Nodo *nodoDerecha = new Nodo(derecha, mapaResultado, conBateria);
	// nodoDerecha.insertParent(*nodo);

	nodoArriba->giro = girar(nodo->st.orientacion, 0);

	nodoDerecha->giro = girar(nodo->st.orientacion, 1);

	nodoAbajo->giro = girar(nodo->st.orientacion, 2);

	nodoIzquierda->giro = girar(nodo->st.orientacion, 3);

	if (mapaResultado[nodoArriba->st.fila][nodoArriba->st.columna] != 'P' && mapaResultado[nodoArriba->st.fila][nodoArriba->st.columna] != 'M' && nodoArriba->st.fila > 0)
		vecinos.push_back(*nodoArriba);
	if (mapaResultado[nodoIzquierda->st.fila][nodoIzquierda->st.columna] != 'P' && mapaResultado[nodoIzquierda->st.fila][nodoIzquierda->st.columna] != 'M' && nodoIzquierda->st.columna > 0)
		vecinos.push_back(*nodoIzquierda);
	if (mapaResultado[nodoDerecha->st.fila][nodoDerecha->st.columna] != 'P' && mapaResultado[nodoDerecha->st.fila][nodoDerecha->st.columna] != 'M' && nodoDerecha->st.columna < mapaResultado[nodoDerecha->st.fila].size() - 1)
		vecinos.push_back(*nodoDerecha);
	if (mapaResultado[nodoAbajo->st.fila][nodoAbajo->st.columna] != 'P' && mapaResultado[nodoAbajo->st.fila][nodoAbajo->st.columna] != 'M' && nodoAbajo->st.fila < mapaResultado.size() - 1)
		vecinos.push_back(*nodoAbajo);


	return;
}

void ComportamientoJugador::giroToAction(int giro, list<Action> &plan) {
	if (giro == 1) {
		plan.push_back(actTURN_R);
		// cout << "Derecha" << endl;
	}
	else if (giro == -1) {
		plan.push_back(actTURN_L);
		// cout << "Izquierda" << endl;
	}
	else if (giro == 2) {
		plan.push_back(actTURN_R);
		plan.push_back(actTURN_R);
		// cout << "Da la vuelta" << endl;
	}

	// cout << "Sigue recto" << endl;
	plan.push_back(actFORWARD);
}


//---------------------- Implementación de la busqueda en profundidad ---------------------------

// Dado el código en carácter de una casilla del mapa dice si se puede
// pasar por ella sin riegos de morir o chocar.
bool EsObstaculo(unsigned char casilla){
	if (casilla=='P' or casilla=='M')
		return true;
	else
	  return false;
}

// Comprueba si la casilla que hay delante es un obstaculo. Si es un
// obstaculo devuelve true. Si no es un obstaculo, devuelve false y
// modifica st con la posición de la casilla del avance.
bool ComportamientoJugador::HayObstaculoDelante(estado st){
	int fil=st.fila, col=st.columna;

  // calculo cual es la casilla de delante del agente
	switch (st.orientacion) {
		case 0: fil--; break;
		case 1: col++; break;
		case 2: fil++; break;
		case 3: col--; break;
	}

	// Compruebo que no me salgo fuera del rango del mapa
	if (fil<0 or fil>=mapaResultado.size()) return true;
	if (col<0 or col>=mapaResultado[0].size()) return true;

	// Miro si en esa casilla hay un obstaculo infranqueable
	if (!EsObstaculo(mapaResultado[fil][col])){
		// No hay obstaculo, actualizo el parámetro st poniendo la casilla de delante.
    st.fila = fil;
		st.columna = col;
		return false;
	}
	else{
	  return true;
	}
}




struct nodo{
	estado st;
	list<Action> secuencia;
};

struct ComparaEstados{
	bool operator()(const estado &a, const estado &n) const{
		if ((a.fila > n.fila) or (a.fila == n.fila and a.columna > n.columna) or
	      (a.fila == n.fila and a.columna == n.columna and a.orientacion > n.orientacion))
			return true;
		else
			return false;
	}
};


// Implementación de la búsqueda en profundidad.
// Entran los puntos origen y destino y devuelve la
// secuencia de Actiones en plan, una lista de Actiones.
bool ComportamientoJugador::pathFinding_Profundidad(const estado &origen, const estado &destino, list<Action> &plan) {
	//Borro la lista
	// cout << "Calculando plan\n";
	plan.clear(); // Se limpia el plan creado en el paso anterior
	set<estado,ComparaEstados> generados; // lista de Cerrados
	stack<nodo> pila;											// lista de Abiertos

  nodo current; // Punto que se irá moviendo
	current.st = origen;
	current.secuencia.empty();

	pila.push(current); //Añadimos el punto de origen en la pila

  while (!pila.empty() and (current.st.fila!=destino.fila or current.st.columna != destino.columna)){
	  	// Avanzamos un paso
		pila.pop(); // Se quita el punto de la pila
		generados.insert(current.st); // Y se añade a los generados

		// Generar descendiente de girar a la derecha
		nodo hijoTurnR = current;
		hijoTurnR.st.orientacion = (hijoTurnR.st.orientacion+1)%4;
		if (generados.find(hijoTurnR.st) == generados.end()){
			hijoTurnR.secuencia.push_back(actTURN_R);
			pila.push(hijoTurnR);

		}

		// Generar descendiente de girar a la izquierda
		nodo hijoTurnL = current;
		hijoTurnL.st.orientacion = (hijoTurnL.st.orientacion+3)%4;
		if (generados.find(hijoTurnL.st) == generados.end()){
			hijoTurnL.secuencia.push_back(actTURN_L);
			pila.push(hijoTurnL);
		}

		// Generar descendiente de avanzar
		nodo hijoForward = current;
		if (!HayObstaculoDelante(hijoForward.st)){
			if (generados.find(hijoForward.st) == generados.end()){
				hijoForward.secuencia.push_back(actFORWARD);
				pila.push(hijoForward);
			}
		}

		// Tomo el siguiente valor de la pila
		if (!pila.empty()){
			current = pila.top();
		}
	}

  cout << "Terminada la busqueda\n";

	if (current.st.fila == destino.fila and current.st.columna == destino.columna){
		cout << "Cargando el plan\n";
		plan = current.secuencia;
		cout << "Longitud del plan: " << plan.size() << endl;
		PintaPlan(plan);
		// ver el plan en el mapa
		VisualizaPlan(origen, plan);
		return true;
	}
	else {
		cout << "No encontrado plan\n";
	}


	return false;
}


estado ComportamientoJugador::Alinear(const estado &origen, const estado &destino, list<Action> &plan) {

	int x;
	int y;
	estado girado = origen;

	if (destino.columna == origen.columna) {
		x = 0;
	}
	else if (destino.columna > origen.columna ) {
		x = 1;
	}
	else {
		x = -1;
	}

	if (destino.fila == origen.fila) {
		y = 0;
	}
	else if (destino.fila > origen.fila ) {
		y = -1;
	}
	else {
		y = 1;
	}

	// cout << "ALINEANDO: " << "O-" << origen.orientacion << "(" << x << " " << y << ")" << endl;
	switch (origen.orientacion)
	{
	case 0:
		if (y == 1) {
			return origen;
		}
		else if (x == 1) {
			plan.push_back(actTURN_R);
			girado.orientacion = (origen.orientacion + 1) % 4;
			// cout << "Derecha" << endl;
		}
		else if (x == -1) {
			plan.push_back(actTURN_L);
			girado.orientacion = (origen.orientacion - 1) % 4;
			// cout << "Izquierda" << endl;
		}
		else if (x == 0) {
			plan.push_back(actTURN_R);
			// cout << "Derecha" << endl;
			plan.push_back(actTURN_R);
			girado.orientacion = (origen.orientacion + 2) % 4;
			// cout << "Derecha" << endl;
		}
		break;
	case 1:
		if (x == 1) {
			return origen;
		}
		else if (y == 1) {
			plan.push_back(actTURN_L);
			girado.orientacion = (origen.orientacion - 1) % 4;
			// cout << "Izquierda" << endl;
		}
		else if (y == -1) {
			plan.push_back(actTURN_R);
			girado.orientacion = (origen.orientacion + 1) % 4;
			// cout << "Derecha" << endl;
		}
		else if (y == 0) {
			plan.push_back(actTURN_R);
			// cout << "Derecha" << endl;
			plan.push_back(actTURN_R);
			girado.orientacion = (origen.orientacion + 2) % 4;
			// cout << "Derecha" << endl;
		}
		break;
	case 2:
		if (y == -1) {
			return origen;
		}
		else if (x == 1) {
			plan.push_back(actTURN_L);
			girado.orientacion = (origen.orientacion - 1) % 4;
			// cout << "Izquierda" << endl;
		}
		else if (x == -1) {
			plan.push_back(actTURN_R);
			girado.orientacion = (origen.orientacion + 1) % 4;
			// cout << "Derecha" << endl;
		}
		else if (x == 0) {
			plan.push_back(actTURN_R);
			// cout << "Derecha" << endl;
			plan.push_back(actTURN_R);
			girado.orientacion = (origen.orientacion + 2) % 4;
			// cout << "Derecha" << endl;
		}
		break;
	case 3:
		if (x == -1) {
			return origen;
		}
		else if (y == 1) {
			plan.push_back(actTURN_L);
			girado.orientacion = (origen.orientacion - 1) % 4;
			// cout << "Izquierda" << endl;
		}
		else if (y == -1) {
			plan.push_back(actTURN_R);
			girado.orientacion = (origen.orientacion + 1) % 4;
			// cout << "Derecha" << endl;
		}
		else if (y == 0) {
			plan.push_back(actTURN_R);
			// cout << "Derecha" << endl;
			plan.push_back(actTURN_R);
			girado.orientacion = (origen.orientacion + 2) % 4;
			// cout << "Derecha" << endl;
		}
		/* code */
		break;

	}

	return girado;

}

bool ComportamientoJugador::pathFinding_Anchura(const estado &origen, const estado &destino, list<Action> &plan) {
	//Borro la lista
	cout << "Calculando plan\n";
	plan.clear();

  nodo current;
	current.st = origen;
	current.secuencia.empty();

	current.st = Alinear(current.st, destino, plan);
	cout << "Alineado con éxito\n";

	while (destino.fila != current.st.fila && destino.columna != current.st.columna) {
		plan.push_back(actFORWARD);
		// cout << "O-" << "(" << current.st.columna << " " << current.st.fila << ")" << "D-" << "(" << destino.columna << " " << destino.fila << ")" << endl;
		if (current.st.orientacion == 0) current.st.fila--;
		else if (current.st.orientacion == 2) current.st.fila++;
		else if (current.st.orientacion == 1) current.st.columna++;
		else if (current.st.orientacion == 3) current.st.columna--;
	}

	cout << "Alineado con éxito\n";
	current.st = Alinear(current.st, destino, plan);

	while (destino.fila != current.st.fila || destino.columna != current.st.columna) {
		plan.push_back(actFORWARD);
		// cout << "R"<< current.st.orientacion << endl;
		// cout << "O-" << "(" << current.st.columna << " " << current.st.fila << ")" << "D-" << "(" << destino.columna << " " << destino.fila << ")" << endl;
		if (current.st.orientacion == 0) current.st.fila--;
		else if (current.st.orientacion == 2) current.st.fila++;
		else if (current.st.orientacion == 1) current.st.columna++;
		else if (current.st.orientacion == 3) current.st.columna--;

	}

	cout << "Terminada la busqueda\n";

	if (current.st.fila == destino.fila and current.st.columna == destino.columna){
		cout << "Cargando el plan\n";
		cout << "Longitud del plan: " << plan.size() << endl;
		PintaPlan(plan);
		// ver el plan en el mapa
		VisualizaPlan(origen, plan);
		return true;
	}
	else {
		cout << "No encontrado plan\n";
	}


	return false;
}

// Sacar por la términal la secuencia del plan obtenido
void ComportamientoJugador::PintaPlan(list<Action> plan) {
	auto it = plan.begin();
	while (it!=plan.end()){
		if (*it == actFORWARD){
			cout << "A ";
		}
		else if (*it == actTURN_R){
			cout << "D ";
		}
		else if (*it == actTURN_L){
			cout << "I ";
		}
		else {
			cout << "- ";
		}
		it++;
	}
	cout << endl;
}



void AnularMatriz(vector<vector<unsigned char> > &m){
	for (int i=0; i<m[0].size(); i++){
		for (int j=0; j<m.size(); j++){
			m[i][j]=0;
		}
	}
}


// Pinta sobre el mapa del juego el plan obtenido
void ComportamientoJugador::VisualizaPlan(const estado &st, const list<Action> &plan){
  AnularMatriz(mapaConPlan);
	estado cst = st;

	auto it = plan.begin();
	while (it!=plan.end()){
		if (*it == actFORWARD){
			switch (cst.orientacion) {
				case 0: cst.fila--; break;
				case 1: cst.columna++; break;
				case 2: cst.fila++; break;
				case 3: cst.columna--; break;
			}
			mapaConPlan[cst.fila][cst.columna]=1;
		}
		else if (*it == actTURN_R){
			cst.orientacion = (cst.orientacion+1)%4;
		}
		else {
			cst.orientacion = (cst.orientacion+3)%4;
		}
		it++;
	}
}



int ComportamientoJugador::interact(Action Action, int valor){
  return false;
}
