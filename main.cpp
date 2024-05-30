//-------------------main.c++--------------------

#include "assistant.h"
#include "pv_porcupine.h"
#include "vosk_api.h"
#include <alsa/asoundlib.h>
#include <curl/curl.h>// Подключите библиотеку cURL для HTTP-запросов
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>// Подключите библиотеку JSON для работы с результатами Vosk
#include <regex>
#include <sstream>
#include <unordered_set>

#define SAMPLE_RATE 16000
#define NUM_CHANNELS 1
#define FRAME_LENGTH 512

using namespace assistant;

#define t_const 1000

//------------------global variables----------------
string input;

int pos , l_pos, cnt = 0;
string m_word , s_word;
int s_count = 0;

string user_name;

//---------------------all functions-------------------------

void check();   //compare the user input with defined commands
void line();      // create new line
void repeat();     //ask user input again.
void shutdown_timer(int);
//void display(int);
void player(string);  //search the song from file(songs.txt) then play the song.
void help();          //show commands
void hacking();
void install(string); // create music folders
void block(string);  //block the websites
void openf(string);   // open the file directory
void lists(string);   //show song list
void convert(string&);   //convert 'space( )' to 'underscore'( _ ) and lowercase string
void srch(string, string extra = "");
void update_song(string);   //copy song name from different files(list.txt) file into one file(songs.txt)
void settings(); // user settings


#include "pv_porcupine.h"
#include "vosk_api.h"
#include <alsa/asoundlib.h>
#include <curl/curl.h>// Подключите библиотеку cURL для HTTP-запросов
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>// Подключите библиотеку JSON для работы с результатами Vosk
#include <regex>
#include <sstream>
#include <unordered_set>

#define SAMPLE_RATE 16000
#define NUM_CHANNELS 1
#define FRAME_LENGTH 512

const char* access_key = "8hu5VS6YtfRL+TZT6H6LKAcsXc2f0GxkHQsLGwvWd1IT23RtqnMDpg==";
const char* keyword_path = "/home/silenth/testVoskApi/Компьютер_ru_linux_v3_0_0.ppn";
const char* model_file_path = "/home/silenth/porcupine/lib/common/porcupine_params_ru.pv";
const char* model_path = "/home/silenth/testVoskApi/model_small";
const char* weather_api_key = "8b64246bdd074b4f8a710502242705"; // Ваш API-ключ OpenWeatherMap

const std::unordered_set<std::string> city_endings = {"е", "и", "у", "а", "ом"};

// Функция для воспроизведения WAV файла
void play_wav(const std::string& file_path) {
  snd_pcm_t *playback_handle;
  snd_pcm_hw_params_t *hw_params;
  int err;
  unsigned int sample_rate = 44100; // Задайте частоту дискретизации, соответствующую вашему файлу
  int channels = 2; // Задайте количество каналов (1 для моно, 2 для стерео)

  if ((err = snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    std::cerr << "Cannot open audio device: " << snd_strerror(err) << std::endl;
    return;
  }

  snd_pcm_hw_params_alloca(&hw_params);
  snd_pcm_hw_params_any(playback_handle, hw_params);
  snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_rate(playback_handle, hw_params, sample_rate, 0);
  snd_pcm_hw_params_set_channels(playback_handle, hw_params, channels);

  if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
    std::cerr << "Cannot set parameters: " << snd_strerror(err) << std::endl;
    snd_pcm_close(playback_handle);
    return;
  }

  std::ifstream wav_file(file_path, std::ios::binary);
  if (!wav_file) {
    std::cerr << "Cannot open WAV file: " << file_path << std::endl;
    snd_pcm_close(playback_handle);
    return;
  }

  wav_file.seekg(44); // Пропустите заголовок WAV файла

  char buffer[4096];
  while (!wav_file.eof()) {
    wav_file.read(buffer, sizeof(buffer));
    std::streamsize bytes_read = wav_file.gcount();
    if (bytes_read > 0) {
      snd_pcm_writei(playback_handle, buffer, bytes_read / 4);
    }
  }

  snd_pcm_drain(playback_handle);
  snd_pcm_close(playback_handle);
}

// Функция инициализации Porcupine
pv_porcupine_t* init_porcupine() {
  pv_porcupine_t *porcupine;
  const float sensitivity = 1.f;
  pv_status_t status = pv_porcupine_init(access_key, model_file_path, 1, &keyword_path, &sensitivity, &porcupine);
  if (status != PV_STATUS_SUCCESS) {
    std::cerr << "Failed to initialize Porcupine: " << pv_status_to_string(status) << std::endl;
    exit(1);
  }
  return porcupine;
}

// Функция инициализации ALSA
snd_pcm_t* init_alsa() {
  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;
  int err;

  if ((err = snd_pcm_open(&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    std::cerr << "Cannot open audio device: " << snd_strerror(err) << std::endl;
    exit(1);
  }

  snd_pcm_hw_params_alloca(&hw_params);
  snd_pcm_hw_params_any(capture_handle, hw_params);
  snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_rate(capture_handle, hw_params, SAMPLE_RATE, 0);
  snd_pcm_hw_params_set_channels(capture_handle, hw_params, NUM_CHANNELS);

  if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
    std::cerr << "Cannot set parameters: " << snd_strerror(err) << std::endl;
    exit(1);
  }

  if ((err = snd_pcm_prepare(capture_handle)) < 0) {
    std::cerr << "Cannot prepare audio interface: " << snd_strerror(err) << std::endl;
    exit(1);
  }

  return capture_handle;
}

// Функция для чтения данных с ALSA
int read_audio_data(snd_pcm_t* capture_handle, int16_t* buffer) {
  int err = snd_pcm_readi(capture_handle, buffer, FRAME_LENGTH);
  if (err != FRAME_LENGTH) {
    std::cerr << "Read from audio interface failed: " << snd_strerror(err) << std::endl;
    exit(1);
  }
  return err;
}

// Функция для выполнения команд
void execute_command(const std::string& command) {
  if (command == "включи свет") {
    std::cout << "Включение света..." << std::endl;
    // Реализация включения света
  } else if (command == "выключи свет") {
    std::cout << "Выключение света..." << std::endl;
    // Реализация выключения света
  } else {
    std::cout << "Неизвестная команда: " << command << std::endl;
  }
}

// Функция для выполнения поиска информации
void search_information(const std::string& query) {
  std::cout << "Поиск информации: " << query << std::endl;
  // Здесь можно реализовать поиск информации, например, используя внешние API
}

std::string remove_city_ending(const std::string& city) {
  for (const auto& ending : city_endings) {
    if (city.size() > ending.size() && city.substr(city.size() - ending.size()) == ending) {
      return city.substr(0, city.size() - ending.size());
    }
  }
  return city;
}

// Функция обратного вызова для curl
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
  size_t totalSize = size * nmemb;
  s->append(static_cast<char*>(contents), totalSize);
  return totalSize;
}

// Функция получения информации о погоде
std::string get_weather(const std::string& city) {
  CURL* curl;
  CURLcode res;
  std::string readBuffer;
  std::string apiKey = "8b64246bdd074b4f8a710502242705";
  std::string url = "http://api.weatherapi.com/v1/current.json?key=" + apiKey + "&q=" + city + "&aqi=no";

  std::cout << "URL: " << url << std::endl; // Вывод URL для проверки

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  std::cout << "API Response: " << readBuffer << std::endl; // Вывод содержимого readBuffer для проверки

  // Проверка на пустой ответ
  if (readBuffer.empty()) {
    return "Ошибка: пустой ответ от сервера погоды";
  }

  // Разбор JSON-ответа
  Json::CharReaderBuilder readerBuilder;
  Json::Value root;
  std::string errs;

  std::istringstream s(readBuffer);
  if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
    std::cerr << "Error parsing JSON: " << errs << std::endl;
    return "Ошибка при разборе ответа от сервера погоды";
  }

  // Проверка на наличие ключевых полей в JSON
  if (!root.isMember("location") || !root.isMember("current")) {
    return "Ошибка: некорректный формат ответа от сервера погоды";
  }

  // Извлечение информации из JSON
  std::string location = root["location"]["name"].asString();
  std::string country = root["location"]["country"].asString();
  double temp_c = root["current"]["temp_c"].asDouble();
  std::string condition = root["current"]["condition"]["text"].asString();
  double wind_kph = root["current"]["wind_kph"].asDouble();
  int humidity = root["current"]["humidity"].asInt();
  double feelslike_c = root["current"]["feelslike_c"].asDouble();

  // Формирование строки с информацией о погоде
  std::ostringstream weather_info;
  weather_info << "Местоположение: " << location << ", " << country << "\n"
               << "Температура: " << temp_c << "°C\n"
               << "Состояние: " << condition << "\n"
               << "Скорость ветра: " << wind_kph << " км/ч\n"
               << "Влажность: " << humidity << "%\n"
               << "Ощущается как: " << feelslike_c << "°C\n";

  return weather_info.str();
}

void process_audio(snd_pcm_t* capture_handle, pv_porcupine_t* porcupine, VoskRecognizer* recognizer) {
  int16_t buffer[FRAME_LENGTH];
  bool is_wake_word_detected = false;
  int err;

  while (true) {
    err = read_audio_data(capture_handle, buffer);

    int32_t result;
    pv_status_t status = pv_porcupine_process(porcupine, buffer, &result);
    if (status != PV_STATUS_SUCCESS) {
      std::cerr << "Failed to process audio: " << pv_status_to_string(status) << std::endl;
      exit(1);
    }

    if (result >= 0) {
      std::cout << "Wake word detected!" << std::endl;
      play_wav("/home/silenth/testVoskApi/jarvis-og/greet1.wav"); // Замените путь на путь к вашему аудио файлу
      is_wake_word_detected = true;
    }

    // Передача данных в Vosk, если isListening = true
    if (is_wake_word_detected && vosk_recognizer_accept_waveform(recognizer, (const char*)buffer, err * 2)) {
      const char* result_json = vosk_recognizer_result(recognizer);
      std::cout << result_json << std::endl;
      is_wake_word_detected = false;
    }
  }
}

int main() {
  vosk_set_log_level(0);
  VoskModel *model = vosk_model_new(model_path);
  if (!model) {
    std::cerr << "Failed to load the model" << std::endl;
    return -1;
  }

  pv_porcupine_t *porcupine = init_porcupine();
  snd_pcm_t *capture_handle = init_alsa();
  VoskRecognizer *recognizer = vosk_recognizer_new(model, SAMPLE_RATE);
  if (!recognizer) {
    std::cerr << "Failed to create recognizer" << std::endl;
    return -1;
  }

  process_audio(capture_handle, porcupine, recognizer);

  vosk_recognizer_free(recognizer);
  vosk_model_free(model);
  snd_pcm_drain(capture_handle);
  snd_pcm_close(capture_handle);
  pv_porcupine_delete(porcupine);

  return 0;
}


//int main()
//{
//  porcupine = init_porcupine();
//  capture_handle = init_alsa();
//  recognizer = vosk_recognizer_new(model_path, SAMPLE_RATE);
//
//  if (!porcupine || !capture_handle || !recognizer) {
//    std::cerr << "Failed to initialize Porcupine, ALSA, or Vosk" << std::endl;
//    return 1;
//  }
//
//  while (true) {
//    process_audio(capture_handle, porcupine, recognizer);
//  }
//
//  pv_porcupine_delete(porcupine);
//  snd_pcm_drain(capture_handle);
//  snd_pcm_close(capture_handle);
//  vosk_recognizer_free(recognizer);
//
//  return 0;
//
//}

//-----------------------------Repeat function-------------------------//
void repeat()
{
	// system("color a");
	system("clear");
	local_clock();
	cout << " \n\n\nType Here  ---> ";
	cin.clear();
	getline(cin, input);  // get command from user

	pos = input.find(" ");
	m_word = input.substr(0, pos); //main command word
	l_pos = input.find('\0');
	s_word = input.substr(pos + 1, l_pos); //rest word


//-----------check conditions--------------//
	check();
}

//----------------------------check function---------------------------//
void check()
{
	if (m_word == "hi" || m_word == "hey" || m_word == "hello" || m_word == "hlo")
	{
		typing("Hi " + user_name + ", how can I help you..");
	}
	else if (m_word == "play")
	{
		if (input == "play" || input == "play " || s_word == " " || s_word == "  " || s_word == "   ")
		{
			speak("Sorry " + user_name + " ,you does not enter song name");
			// tutorial("play");
		}
		else
		{
			player(s_word);
		}
	}

	else if (m_word == "update" || m_word == "updating")
	{
		system("clear");
		line();
		typing("Updating the song list...");
		usleep(t_const * 100);
		system("clear");
		line();
		typing("Please wait");
		update_song("english");
                update_song("russian");
		update_song("others");
		system("clear");
		remove("data/songs.txt");
		rename("data/temp.txt", "data/songs.txt");
		line();
		typing("All songs are updated in the file");
	}
	else if (input == "exit" || input == "q" || input == "quit")
	{
		speak("Good bye," + user_name);
		usleep(t_const * 600);
		cout << "\n\n\n\n\n\n\t\t\t\t\t";
		typing("Created By : silenthnowakeup");
		usleep(t_const * 1000);
		system("clear");
		exit(1);
	}
	else if (input == "find ip" || input == "find my ip" || m_word == "ip")
	{
		typing("Finding your IP address");
		system("ipconfig");
		system("pause");
	}
	else if (m_word == "shutdown" || m_word == "restart")
	{
		typing("Your Pc will ");
		typing(m_word);
		shutdown_timer(5);
		speak("Now , I am going to sleep");
		if (m_word == "shutdown")
			system("shutdown /s");
		else
			system("shutdown /r");
		usleep(t_const * 10);
		exit(1);
	}

	else if (m_word == "what" || m_word == "who" || m_word == "how" || m_word == "when" || m_word == "where" || m_word == "why")
	{
		if (input == "what is your name")
		{
			typing("My name is silence.");
		}
		else if (input == "who are you" || input == "who created you" || input == "who made you")
		{
			typing("I am Silence, a virtual assistant");
			usleep(t_const * 300);
			line();
			typing("I was created today");
			usleep(t_const * 300);
		}
		else
			srch(input);
	}

	else if (input == "settings" || input == "setting" || input == "set")
		settings();

	else if (m_word == "song" || m_word == "music")
		srch(s_word, "song");

	else if (input == "install")
	{
		system("mkdir My_beat");

		install("russian");
		install("english");
		install("others");
		cout << "\nCreating folders...";
		usleep(t_const * 200);
		cout << "\nCreating files...";
		usleep(t_const * 200);
		system("clear");

		typing("\nAll files are installed");
		usleep(t_const * 300);
	}

	else if (input == "help")
		help();

	else if (m_word == "movie")
		srch(s_word, "movie");

	else if (m_word == "pdf")
		srch(s_word, "pdf");

	else if (m_word == "search")
		srch(s_word);

	else if (m_word == "cmd")
		system(s_word.c_str());

	else if (input == "start hacking" || input == "hacking lab" || input == "hackingzone" || input == "hacking tools" || m_word == "hack")
	{
		hacking();
	}

	else if (m_word == "list")
	{
		if (s_word == "all songs" || s_word == "songs")
			lists("data/songs.txt");
	}
	/*else if(input=="unhide data"||input=="unhide data folder")
	      {
	        system("attrib -s -h data");
	      }
	else if(input=="hide data"||input=="hide data folder")
	      {
	        system("attrib +s +h data");
	      }
	      */

	else if (m_word == "block")
	{
		block(s_word);
	}
	else if (m_word == "yt" || m_word == "youtube" || m_word == "watch")
	{
		srch(s_word, "youtube");
	}

	else if (m_word == "open")
	{
		if (s_word == "chrome" || s_word == "google chrome")
		{
			system("chrome");
		}
		else if (s_word == "mozilla" || s_word == "firefox")
		{
			system("firefox");
		}
		else
			openf(s_word);
	}

	else
	{
		speak("Sorry " + user_name + ", unknown command...");
		cnt++;
		if (cnt >= 3)
		{	usleep(t_const * 600);
			speak("I think ");
			usleep(t_const * 500);
			speak("you are a new user");
			usleep(t_const * 600);
			speak("You need some help...");
			help();
		}
	}

	usleep(t_const * 700);
	repeat();
}

//-----------------------user settings---------------------
void settings()
{
	string un;
	int ss, sa, sp, ts;
        string v;

	cout << "\n\n";
	typing("Enter data in the following given format:\n");
	cout << "\n1.user name(single word only)";
	cout << "\n2.speak speed(in WPM)";
	cout << "\n3.speak volume(0-200)";
	cout << "\n4.speak pitch(0-99)";
	cout << "\n5.typing speed(in ms)";
        cout << "\n6.voice name(english-us, english, hindi, russian, others)";
	cout << "\n\nExample 1:\n";
	cout << "Silenth 160 100 40 40 english-us";
	cout << "\n\nExample 2:\n";
	cout << "Silenth 150 120 60 30 english";
	cout << "\n\n\n---> ";
	cin >> un >> ss >> sa >> sp >> ts >> v;
	if ((sa <= 200 && sa > 0) && (sp <= 99 && sp > 0 ))
	{
		save_settings(un, ss, sa, sp, ts, v);
		typing("Restart silence to see changes");

	}
	else
	{
		speak("Something went wrong");
	}
	cin.ignore('\n');
}

//------------------------------player function------------------------//
void player(string item)
{
	ifstream file;
	string song_name;
	song_name = item;
	convert(item);
	char song[30], singer[30];
	char path[90] = "xdg-open My_beat/";
	file.open("data/songs.txt", ios::app);
	while (file >> song >> singer)
	{
		if (song == item)
		{
			typing("Playing the song ");
			usleep(t_const * 150);
			typing(song_name);
			strcat(path, singer);
			strcat(path, "/");
			strcat(path, song);
			strcat(path, ".mp3");
			system(path);
			break;
		}
	}
	usleep(t_const * 150);
	system("clear");
	//--------------if song not found------------------
	if (song != item)
	{
		typing(song_name);
		typing(" not found.");
		if (s_count % 3 == 0)
		{
			usleep(t_const * 200);
			system("clear");
			speak("But you can download the song by using the command");
			usleep(t_const * 1300);
			line();
			typing("song ");
			typing(song_name);
		}
		s_count++;
	}

	file.close();
}

//------------------------------convert string-------------------------//
void convert(string &c)
{

	for (int i = 0; c[i] != '\0'; i++)
	{
		if (c[i] == ' ')
			c[i] = '_';
		c[i] = tolower(c[i]);
	}
}

//------------------------------update the song list-------------------//
void update_song(string name)
{
	fstream a, b;
	char word[20], old[20];
	system("clear");
	a.open("My_beat/" + name + "/list.txt");
	b.open("data/temp.txt", ios::app | ios::ate);
	while (a >> word)
	{
		b << word << " " << name << "\n";
	}
	b.close();
	a.close();
}

//------------------------timer function--------------

// function to display the timer
void displayClock(int seconds)
{
	int h, m;
	h = m = 0;
	// system call to clear the screen
	system("clear");
	cout << "\n\n";
	cout << setfill(' ') << setw(75) << "	        TIMER	      	\n";
	cout << setfill(' ') << setw(75) << " --------------------------\n";
	cout << setfill(' ') << setw(29);
	cout << "| " << setfill('0') << setw(2) << h << " hrs | ";
	cout << setfill('0') << setw(2) << m << " min | ";
	cout << setfill('0') << setw(2) << seconds << " sec |" << endl;
	cout << setfill(' ') << setw(75) << " --------------------------\n";
}

void shutdown_timer(int t)
{
	while (t)
	{
		displayClock(t);
		usleep(t_const * 900);
		t--;
	}
}

//----------------online srch and also 0download songs----------------

void srch(string query, string extra)
{
	for (int i = 0; query[i] != '\0'; i++)
	{
		if (query[i] == ' ')
			query[i] = '+';
	}

	usleep(t_const * 200);
	system("clear");
	line();
	typing("Cheking internet connection...");
	if (s_count % 5 == 0)
	{
		line();
		usleep(t_const * 90);
		cout << "Colleting information..\n";
		usleep(t_const * 50);
		cout << "securing the data..\n";
		usleep(t_const * 30);
		cout << "clear the cookies..\n";
		usleep(t_const * 100);
		system("ipconfig");
		line();
		typing("All protocols are secured...");
	}

	usleep(t_const * 250);
	speak("Connecting to your browser.");
	string url;

	if (extra == "youtube")
	{
		url = "xdg-open https://www.youtube.com/results?search_query=";
		url += query;
		system(string(url).c_str());
	}
	else if (extra == "song")
	{
		url = "xdg-open https://m.soundcloud.com/search?q=";
		url += query;
		system(string(url).c_str());
	}
	else if (extra == "pdf")
	{
		url = "xdg-open https://www.google.com/search?q=filetype%3Apdf+";
		url += query;
		system(string(url).c_str());
	}
	else if (extra == "movie")
	{

		url = "xdg-open https://kinogo.biz/search/";
		url += query;
		url += "+-inurl%3A(htm%7Chtml%7Cphp%7Cpls%7Ctxt)+intitle%3Aindeof+%22last+modified%22(mp4%7Cmkv%7Cwma%7Caac%7Cavi)";
		system(string(url).c_str());
	}
	else
	{
		url = "xdg-open https://www.google.com/search?q=";
		url += query;
		system(string(url).c_str());
	}

	s_count++;
}
//-----------------newline function----------------
void line()
{
	cout << "\n";
}

//----------------install files---------------------
void install(string fold)
{
	fstream file;
	string foldname, filename;
	foldname = "mkdir My_beat/";

	foldname += fold;
	system(string(foldname).c_str());

	filename = "My_beat/";
	filename += fold;
	filename += "/list.txt";
	file.open(filename, ios::app);
	file.close();
}

//---------------list the file texts------------------
void lists(string link)
{
	fstream file;
	int cnt = 0;
	char word[50], old[50];
	file.open(link);
	while (file >> word >> old)
	{
		cnt++;
		cout << word << "\n";
	}
	cout << "\n\t\tTotal songs available :" << cnt << endl;
	string p, s = "Only ";
	p = cnt;
	s += p;
	s += "songs are available";
	if (cnt != 0)
	{
		speak(s);
	}
	file.close();
	system("pause");
}

//----------------------hacking zone------------------
void hacking()
{
	system("clear");
	system("color f");
	speak("You are Welcome in the Hacking Lab");
	cout << setfill(' ') << setw(50) << "      ________       \n";
	usleep(t_const * 100);
	cout << setfill(' ') << setw(50) << "     |        |      \n";
	usleep(t_const * 100);
	cout << setfill(' ') << setw(50) << "     |  #   # |      \n";
	usleep(t_const * 100);
	cout << setfill(' ') << setw(50) << "     |  #   # |      \n";
	usleep(t_const * 100);
	cout << setfill(' ') << setw(50) << "     |   # #  |      \n";
	usleep(t_const * 100);
	cout << setfill(' ') << setw(50) << "     |    #   |      \n";
	usleep(t_const * 100);
	cout << setfill(' ') << setw(50) << "     |________|      \n";
	line();
	usleep(t_const * 1000);
	system("color c");
	cout << setfill(' ') << setw(50) << " Silenth Hacking Lab  \n";
	usleep(t_const * 1000);
	line();
	typing("Still in development...");
}

void block(string website)
{
	fstream file;
	file.open("C:/WINDOWS/system32/drivers/etc/hosts", ios::app);
	file << "\n127.0.0.2 www." << website;
	typing("Blocking the website..");
	file.close();
}

void openf(string location)
{
	string path = "xdg-open ", item = location;
	convert(item);
	path += item;
	typing("Open....");
	typing(location);
	system(string(path).c_str());
}

//----------------------help file---------------------
void help()
{
	cnt = 0;
	speak("you can use only these commands");
	system("clear");
	cout << "\n\n";
	system("color f");
	line();
	cout << setfill(' ') << setw(75) << "----------------------------\n";
	cout << setfill(' ') << setw(75) << "          Commands          \n";
	cout << setfill(' ') << setw(75) << "--------------------------\n\n";
	cout << setfill(' ') << setw(75) << "    1.search (any question)  \n";
	cout << setfill(' ') << setw(75) << "    2.open (google,mozilla)  \n";
	cout << setfill(' ') << setw(75) << "    3.block (website name)   \n";
	cout << setfill(' ') << setw(75) << "    4.song (song name)       \n";
	cout << setfill(' ') << setw(75) << "    5.update                 \n";
	cout << setfill(' ') << setw(75) << "    6.watch (videoname)      \n";
	cout << setfill(' ') << setw(75) << "    7.pdf (pdfname)          \n";
	cout << setfill(' ') << setw(75) << "    8.movie (moviename)      \n";
	cout << setfill(' ') << setw(85) << "    9.what/how/where/who/why (question)\n";
	cout << setfill(' ') << setw(75) << "   10.cmd (cmd commands)     \n";
	cout << setfill(' ') << setw(75) << "   11.find my ip             \n";
	cout << setfill(' ') << setw(75) << "   12.play (song name)       \n";
	cout << setfill(' ') << setw(75) << "   13.list songs             \n";
	cout << setfill(' ') << setw(75) << "   10.exit/quit/q            \n";
	cout << setfill(' ') << setw(75) << "   11.shutdown/restart       \n";
	cout << setfill(' ') << setw(75) << "   12.install                \n";
        cout << "Press any key to continue...";
        getchar();
}
