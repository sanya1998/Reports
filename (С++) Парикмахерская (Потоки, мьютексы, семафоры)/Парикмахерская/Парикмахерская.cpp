#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include <queue>

DWORD WINAPI Hairdresser_function(LPVOID lPparam);
DWORD WINAPI Client_function(LPVOID lPparam);

std::queue<int> table_hall;// В зале ожидания
std::queue<int> table_room;// В рабочей комнате

HANDLE hNomerok_hall;
HANDLE hNomerok_room;

HANDLE hFreeArmchairs;
HANDLE hReady;
HANDLE* hHaircutFinish;
HANDLE hLeaveArmchair;

int times, all_places;

int main()
{
	setlocale(LC_ALL,"RUS");
	int amount_hairdressers, amount_clients, amount_sofas;
	std::cin >> amount_clients >> times >> amount_hairdressers >> amount_sofas;

	all_places = amount_hairdressers + amount_sofas;
	
	for (int i = 0; i < all_places; i++)
		table_hall.push(i + 1);

	hFreeArmchairs = CreateSemaphore(NULL, amount_hairdressers, amount_hairdressers, NULL);
	hReady = CreateSemaphore(NULL, 0, amount_hairdressers, NULL);
	
	hHaircutFinish = new HANDLE[all_places];
	for(int i = 0; i < all_places; i++)
		hHaircutFinish[i] = CreateSemaphore(NULL, 0, 1, NULL);
	
	hLeaveArmchair = CreateSemaphore(NULL, 0, amount_hairdressers, NULL);

	hNomerok_hall = CreateMutex(NULL, FALSE, NULL);
	hNomerok_room = CreateMutex(NULL, FALSE, NULL);

	HANDLE* hHairdresser = new HANDLE[amount_hairdressers];
	for (int i = 0; i < amount_hairdressers; i++)
		hHairdresser[i] = CreateThread(NULL, 0, Hairdresser_function, (LPVOID)i, 0, 0);
	
	HANDLE* hClient = new HANDLE[amount_clients];
	for (int i = 0; i < amount_clients; i++)
		hClient[i] = CreateThread(NULL, 0, Client_function, (LPVOID)i, 0, 0);
	WaitForMultipleObjects(amount_clients, hClient, TRUE, INFINITE);
	// иногда парикмахер начинает печатать сообщение, но не допечатывает его

	// Семафоры
	CloseHandle(hFreeArmchairs);
	CloseHandle(hReady);
	for (int i = 0; i < all_places; i++)
		CloseHandle(hHaircutFinish[i]);
	delete[] hHaircutFinish;
	CloseHandle(hLeaveArmchair);

	// Мьютексы
	CloseHandle(hNomerok_hall);
	CloseHandle(hNomerok_room);

	// Потоки
	for (int i = 0; i < amount_hairdressers; i++)
		CloseHandle(hHairdresser[i]);
	delete[] hHairdresser;

	for (int i = 0; i < amount_clients; i++)
		CloseHandle(hClient[i]);
	delete[] hClient;

	return 0;
}

DWORD WINAPI Hairdresser_function(LPVOID lPparam)
{
	int num = (int)lPparam;
	srand(num);

	while (true) {
		// Ждёт, когда посетитель будет готов
		WaitForSingleObject(hReady, INFINITE);

		// Берет номерок со стола в рабочей комнате
		WaitForSingleObject(hNomerok_room, INFINITE);
		int nomerok = table_room.front();
		table_room.pop();
		ReleaseMutex(hNomerok_room);

		printf("Парикмахер %d стрижёт посетителя с номерком %d\n", num, nomerok);
		Sleep(1000 * (1 + rand() % 3)); // Стрижка
		// Извещает, что стрижка закончена
		ReleaseSemaphore(hHaircutFinish[nomerok-1], 1, NULL); 

		// Ждет, когда посетитель покинет кресло
		WaitForSingleObject(hLeaveArmchair, INFINITE); 

		// Возвращает номерок в зал ожидания
		WaitForSingleObject(hNomerok_hall, INFINITE);
		table_hall.push(nomerok);
		ReleaseMutex(hNomerok_hall);

		printf("Парикмахер %d готов принять следующего посетителя\n", num);
		// Извещает, что кресло освободилось
		ReleaseSemaphore(hFreeArmchairs, 1, NULL); 
	}

	return 0;
}

DWORD WINAPI Client_function(LPVOID lPparam)
{
	int num = (int)lPparam;
	srand(-num-1);
	for (int i = 0, nomerok; i < times; ) {
		
		// Пытается взять номерок
		WaitForSingleObject(hNomerok_hall, INFINITE);
		if (table_hall.empty())
			nomerok = -1;
		else {
			nomerok = table_hall.front();
			table_hall.pop();
		}	
		ReleaseMutex(hNomerok_hall);

		// иногда (else) печатает быстрее, чем (if) => получается не тот порядок
		if (nomerok != -1) {			
			printf("Посетитель %d зашёл в парикмахерскую и взял номерок %d\n", num, nomerok);
			// Ждет извещения о свободном кресле
			WaitForSingleObject(hFreeArmchairs, INFINITE); 
			
			// Кладет номерок на стол в рабочей комнате
			WaitForSingleObject(hNomerok_room, INFINITE);
			table_room.push(nomerok);
			ReleaseMutex(hNomerok_room);
			
			printf("Посетитель %d садится в кресло парикмахера\n", num);
			//шлёт извещение, что готов к стрижке:
			ReleaseSemaphore(hReady, 1, NULL);

			// Ждет извещение, что стрижка закончена:
			WaitForSingleObject(hHaircutFinish[nomerok-1], INFINITE);

			//шлёт извещение, что покинул кресло:
			ReleaseSemaphore(hLeaveArmchair, 1, NULL);
			printf("Посетитель %d постригся и уходит из парикмахерской\n", num);
			
			i++;
		}
		else
			printf("Свободных мест нет. Посетитель %d уходит из парикмахерской\n", num);
		if(i < times)
			Sleep(1000 * (1 + rand() % 5));
	}

	return 0;
}