#ifndef ASSISTANT_H
#define ASSISTANT_H

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <ctype.h>
#include <ctime>
#include <thread>

using namespace std;

//-------------main group--------------
namespace assistant
{
void init();
void load_settings();

void banner();
void greeting();
void local_clock();

void typing(string);
void speak(string);

void save_settings(string, int, int, int, int, string);

string get_uname();
}

#endif // ASSISTANT_H