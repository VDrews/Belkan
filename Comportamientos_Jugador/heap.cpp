#include <iostream>
#include <vector>
#include <omp.h>

using namespace std;

template <class T>
class Heap {
  vector<T> list;

  void bubbleUp();
  void bubbleDown();
  void swap(int child, int parent);
  int getLeftChild(int parent);
  int getRightChild(int parent);
  int getParent(int child);
public:
  Heap();
  bool contains(Nodo el);
  bool empty();
  T front();
  Nodo nearest();
  void insert(T );
  T remove();
  int getSize();

};

template <class T>
Heap<T> :: Heap(){
  
}

template <class T>

// Recorre el heap buscando el nodo pasado por parametro
bool Heap<T> :: contains(Nodo el) {
  #pragma omp parallel 
  {
    #pragma omp for nowait
    for(int i = 0; i < list.size(); ++i) {
      if (list[i] == el) {
        return true;
      }
    }
    return false;
  }
}

// Busca el nodo con menor hCost
// Usado para trocear el plan
template <class T>
Nodo Heap<T> :: nearest() {
  int hCost = list[0].hCost;
  Nodo n(list[0]);
  #pragma omp parallel 
  {
    #pragma omp for nowait
    for(int i = 1; i < list.size(); ++i) {
      if (list[i].hCost <= hCost) {
        n = list[i];
        hCost = list[i].hCost;
      }
    }
    return n;
  }
}

template <class T>
int Heap<T> :: getSize(){
  return list.size();
}

template <class T>
bool Heap<T> :: empty() {
    return getSize() == 0;
}

template <class T>
T Heap<T> :: front() {
    return list[0];
}

// Intercambiamos los elementos de los indices
template <class T>
void Heap<T>::swap(int child, int parent) {
  
  T temp = list[child];
  list[child] = list[parent];
  list[parent] = temp;
  

}

// Obtenemos el padre
template <class T>
int Heap<T> :: getParent(int child) {
  if (child % 2 == 0)
	return (child /2 ) -1;
  else 
	return child/2;
  
}

// Obtenemos el hijo a la izquierda del padre
template <class T>
int Heap<T> :: getLeftChild(int parent){
  return 2*parent +1;
}

// Obtenemos el hijo a la derecha del padre
template <class T>
int Heap<T> :: getRightChild(int parent){
  return 2 * parent + 2;
}

// Inserta un elemento al Heap y lo reordena
template <class T>
void Heap<T> :: insert(T value) {
  list.push_back(value);
  
  bubbleUp();

}

// Sube el elemento mientras que el hijo sea mayor que el padre
template <class T>
void Heap <T>:: bubbleUp() {
  
  int child = list.size() - 1;
  
  int parent = getParent(child);
  
  
  while (child >=0 && parent >= 0 && list[child] > list[parent]) {
	swap(child, parent);
	child = parent;
  
	parent = getParent(child);
  

  }
  
  
}


// Elimina el primer elemento y lo devuelve
template <class T>
T Heap<T> :: remove() {
  int child = list.size()  - 1;
  swap(child, 0);
  
  T value = list.back();
  list.pop_back();
  
  bubbleDown();
  
  return value;
}

template <class T>
void Heap<T> :: bubbleDown() {
  int parent = 0;

  while (1) {
	int left = getLeftChild(parent);
	int right = getRightChild(parent);
	int length = list.size();
	int largest = parent;

	if (left < length && list[left] > list[largest])
	  largest = left;

	if (right < length && list[right] > list[largest])
	  largest = right;

	if (largest != parent) {
	  swap(largest, parent);
	  parent = largest;
	}
	else 
	  break;
	

  }

}