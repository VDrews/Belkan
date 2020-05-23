#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"

#include <list>
#include <iostream>


struct estado {
  int fila;
  int columna;
  int orientacion;
};

class Nodo {
	public:
	estado st;
  bool conBateria = false;
  vector< vector< unsigned char> > mapa;
  bool bikini = false;
  bool zapatillas = false;
	int gCost = 0;
	int hCost = 0;
	Nodo *parent;
	int giro = 0; //Gira a la izquierda si -1 y a la derecha si 1, 2 para dar la vuelta

	Nodo(estado entrada, vector< vector< unsigned char> > &mapaResultado, bool bateria) {
		st = entrada;
    parent = NULL;
    conBateria = bateria;
    mapa = mapaResultado;
    if (mapa[st.fila][st.columna] == 'K') {
      bikini = true;
    }
    else if (mapa[st.fila][st.columna] == 'D') {
      zapatillas = true;
    }
	}

  Nodo(const Nodo &copy) {
    Nodo *padre = copy.parent;
    bikini = copy.bikini;
    conBateria = copy.conBateria;
    zapatillas = copy.zapatillas;
    mapa = copy.mapa;
    st = copy.st;
    gCost = copy.gCost;
    hCost = copy.hCost;
    parent = padre;
    giro = copy.giro;
  }

	int fCost() {
		return (gCost + hCost) - beneficio();
	}

  int beneficio() {
    if (mapa[st.fila][st.columna] == 'X') {
      return 3000 - (gCost + hCost);
    }
    else return 0;
  }

  int costeTerreno(Sensores sensores, bool has_bikini, bool has_zapatillas) {
    if (!conBateria) {
      return 1;
    }

    switch (mapa[st.fila][st.columna])
    {

    case 'A':
      if (bikini) {
        return 10;
      }
      else {
        return 100;
      }
      break;
    case 'B':
      if (zapatillas) {
        return 5;
      }
      else {
        return 50;
      }
      break;
    case 'X':
      if (sensores.bateria <= 400 && sensores.tiempo > 500) return -2000;
      else return -100;
    case 'K':
      if (sensores.nivel == 4 && !has_bikini && !bikini) return -2000; // Como se va a coger objetivos hasta que se acabe el tiempo, cunde cogerlas siempre
      else return 1;
    case 'D':
      if (sensores.nivel == 4 && !has_zapatillas && !zapatillas) return -2000; // Como se va a coger objetivos hasta que se acabe el tiempo, cunde cogerlas siempre
      else return 1;
    case 'T':
      return 2;
    case 'M':
      return NULL;
      break;
    case 'P':
      return NULL;
      break;
    case 'S':
      return 1;
      break;
    case '?':
      // Bajo la indeterminación de no saber que casilla es, lo consideraremos como la media de los costes
      return 40;
      break;
    default:
      return 40;
      break;
    }
  }

  void debug() {
    cout << st.orientacion << "(" << st.columna << " " << st.fila << ")" << "\tGiro: " << giro << "\tG:" << gCost << "\tH:" << hCost;
    if (parent == NULL) {
      cout << "NOPARENT" << endl;
    }
    else {
      cout << "\t";
      parent->debugWithoutParent();
    }
  }

  void debugWithoutParent() {
    cout << st.orientacion << "(" << st.columna << " " << st.fila << ")" << "\tGiro: " << giro << "\tG:" << gCost << "\tH:" << hCost << endl;

  }

  void insertParent(Nodo *nodo) {
    this->parent = nodo;
  }

  Nodo& operator=(Nodo& copy) {
    Nodo *padre = copy.parent;
    st = copy.st;
    conBateria = copy.conBateria;
    gCost = copy.gCost;
    hCost = copy.hCost;
    parent = padre;
    giro = copy.giro;
    return *this;

  }

  bool operator<(const Nodo& b) const
  {
    int itFCost = (gCost + hCost);
    int bCost = (b.gCost + b.hCost);

    bool ret = (itFCost > bCost && itFCost != bCost || hCost > b.hCost);
    return ret;
  }

  bool operator>(const Nodo& b) const
  {
    int itFCost = (gCost + hCost);
    int bCost = (b.gCost + b.hCost);

    bool ret = (itFCost < bCost || itFCost == bCost && hCost < b.hCost);
    return ret;
  }

	bool operator == (const Nodo& s) const { return st.columna == s.st.columna and st.fila == s.st.fila; }
	bool operator != (const Nodo& s) const { return st.columna != s.st.columna or st.fila != s.st.fila; }
	bool transitable(const vector< vector< unsigned char> > &mapa) {
		return mapa[st.fila][st.columna] != 'M' && mapa[st.columna][st.fila] != 'P';
	}
};

class comparaNodos
{
public:
    int operator() (Nodo& n1, Nodo& n2)
    {
        return n1.fCost() > n2.fCost();
    }
};

class ComportamientoJugador : public Comportamiento {
  public:
    ComportamientoJugador(unsigned int size) : Comportamiento(size) {
      // Inicializar Variables de Estado
      fil = col = 99;
      brujula = 0; // 0: Norte, 1:Este, 2:Sur, 3:Oeste
      destino.fila = -1;
      destino.columna = -1;
      destino.orientacion = -1;
    }
    ComportamientoJugador(std::vector< std::vector< unsigned char> > mapaR) : Comportamiento(mapaR) {
      // Inicializar Variables de Estado
      fil = col = 99;
      brujula = 0; // 0: Norte, 1:Este, 2:Sur, 3:Oeste
      destino.fila = -1;
      destino.columna = -1;
      destino.orientacion = -1;
    }
    ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport){}
    ~ComportamientoJugador(){}

    Action think(Sensores sensores);
    int interact(Action Action, int valor);
    void VisualizaPlan(const estado &st, const list<Action> &plan);
    ComportamientoJugador * clone(){return new ComportamientoJugador(*this);}

  private:
    // Declarar Variables de Estado
    int fil, col, brujula;
    estado actual, destino;
    list<Action> plan;
    int Actiones = 0;

    // Métodos privados de la clase
    bool pathFinding(int level, const estado &origen, const estado &destino, list<Action> &plan, Sensores sensores);
    bool pathFinding_Profundidad(const estado &origen, const estado &destino, list<Action> &plan);
    bool pathFinding_Anchura(const estado &origen, const estado &destino, list<Action> &plan);
    estado Alinear(const estado &origen, const estado &destino, list<Action> &plan);
    void PintaPlan(list<Action> plan);
    bool HayObstaculoDelante(estado st);
    bool HayAldeanoDelante(estado st);
    bool EsLimite(estado st);


    bool encontrarCamino(const estado &origen, const estado &destino, list<Action> &plan, bool conBateria, Sensores sensores);
    void RetrazarPlan(Nodo origen, Nodo destino, list<Action> &plan);
    void GetVecinos(Nodo *nodo, list<Nodo> &vecinos, bool conBateria);
    void giroToAction(int giro, list<Action> &plan);
    void mapear(Sensores sensores);
};

#endif
