#include "assistant.h"
#include "pv_porcupine.h"
#include "vosk_api.h"
#include <alsa/asoundlib.h>
#include <jsoncpp/json/json.h>
#include <regex>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

#define SAMPLE_RATE 16000
#define NUM_CHANNELS 1
#define FRAME_LENGTH 512

using namespace assistant;
namespace fs = std::filesystem;

const char* access_key = "8hu5VS6YtfRL+TZT6H6LKAcsXc2f0GxkHQsLGwvWd1IT23RtqnMDpg==";
const char* keyword_path = "/home/silenth/testVoskApi/Компьютер_ru_linux_v3_0_0.ppn";
const char* model_file_path = "/home/silenth/porcupine/lib/common/porcupine_params_ru.pv";
const char* model_path = "/home/silenth/testVoskApi/model_small";
const std::string sound_base_path = "/home/silenth/virtual-assistant/sound/jarvis-remake/";

struct Command {
  std::string action;
  std::string cli_cmd;
  std::vector<std::string> cli_args;
  std::vector<std::string> sounds;
  std::vector<std::string> phrases;
};

std::vector<Command> loadCommandsFromFile(const std::string& filePath);
std::vector<Command> loadCommandsFromDirectory(const std::string& directoryPath);
void executeCommand(const Command& cmd);
void play_wav(const std::string& file_path);
std::string getRandomSound(const std::vector<std::string>& sounds);
pv_porcupine_t* init_porcupine();
snd_pcm_t* init_alsa();
int read_audio_data(snd_pcm_t* capture_handle, int16_t* buffer);
void process_audio(snd_pcm_t* capture_handle, pv_porcupine_t* porcupine, VoskRecognizer* recognizer, const std::vector<Command>& commands);
void savePid(const std::string& filePath);

std::vector<Command> loadCommandsFromFile(const std::string& filePath) {
  std::vector<Command> commands;
  YAML::Node config;
  try {
    config = YAML::LoadFile(filePath);
  } catch (const YAML::BadFile& e) {
    std::cerr << "Failed to load file: " << e.what() << std::endl;
    return commands;
  }

  if (!config["list"]) {
    std::cerr << "Invalid YAML format: 'list' key not found" << std::endl;
    return commands;
  }

  for (const auto& cmdNode : config["list"]) {
    Command cmd;
    try {
      cmd.action = cmdNode["command"]["action"].as<std::string>();
      if (cmd.action == "cli") {
        cmd.cli_cmd = cmdNode["command"]["cli_cmd"].as<std::string>();
        if (cmdNode["command"]["cli_args"]) {
          for (const auto& arg : cmdNode["command"]["cli_args"]) {
            cmd.cli_args.push_back(arg.as<std::string>());
          }
        }
      }
      for (const auto& sound : cmdNode["voice"]["sounds"]) {
        cmd.sounds.push_back(sound.as<std::string>());
      }
      for (const auto& phrase : cmdNode["phrases"]) {
        cmd.phrases.push_back(phrase.as<std::string>());
      }
      commands.push_back(cmd);
    } catch (const YAML::InvalidNode& e) {
      std::cerr << "Invalid node in YAML file: " << e.what() << std::endl;
    } catch (const YAML::Exception& e) {
      std::cerr << "YAML error: " << e.what() << std::endl;
    }
  }
  return commands;
}

std::vector<Command> loadCommandsFromDirectory(const std::string& directoryPath) {
  std::vector<Command> allCommands;
  for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
    if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
      auto commands = loadCommandsFromFile(entry.path().string());
      allCommands.insert(allCommands.end(), commands.begin(), commands.end());
    }
  }
  return allCommands;
}

void executeCommand(const Command& cmd) {
  if (cmd.action == "cli") {
    std::string full_cmd = cmd.cli_cmd;
    for (const auto& arg : cmd.cli_args) {
      full_cmd += " " + arg;
    }
    std::cout << "Executing command: " << full_cmd << std::endl; // Отладочное сообщение
    system(full_cmd.c_str());
  } else if (cmd.action == "voice") {
    if (!cmd.sounds.empty()) {
      std::string sound_file = getRandomSound(cmd.sounds);
      std::cout << "Playing command sound: " << sound_file << std::endl;
      play_wav(sound_base_path + sound_file + ".wav");
    } else {
      std::cout << "No sounds specified for command: " << cmd.action << std::endl;
    }
  }
}

void play_wav(const std::string& file_path) {
  std::cout << "Playing WAV file: " << file_path << std::endl;
  snd_pcm_t *playback_handle;
  snd_pcm_hw_params_t *hw_params;
  int err;
  unsigned int sample_rate = 44100;
  int channels = 2;

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

  wav_file.seekg(44);

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

std::string getRandomSound(const std::vector<std::string>& sounds) {
  if (sounds.empty()) {
    return "";
  }
  int index = rand() % sounds.size();
  return sounds[index];
}

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
    std::cerr << "Cannot prepare audio interface for use: " << snd_strerror(err) << std::endl;
    exit(1);
  }

  return capture_handle;
}

int read_audio_data(snd_pcm_t* capture_handle, int16_t* buffer) {
  int err = snd_pcm_readi(capture_handle, buffer, FRAME_LENGTH);
  if (err == -EPIPE) {
    std::cerr << "Overrun occurred" << std::endl;
    snd_pcm_prepare(capture_handle);
    return -EPIPE;
  } else if (err < 0) {
    std::cerr << "Error reading from audio interface: " << snd_strerror(err) << std::endl;
    return err;
  }
  return 0;
}

void process_audio(snd_pcm_t* capture_handle, pv_porcupine_t* porcupine, VoskRecognizer* recognizer, const std::vector<Command>& commands) {
  int16_t buffer[FRAME_LENGTH];
  bool is_wake_word_detected = false;

  while (true) {
    if (read_audio_data(capture_handle, buffer) != 0) {
      continue;
    }

    int32_t result;
    pv_status_t status = pv_porcupine_process(porcupine, buffer, &result);
    if (status != PV_STATUS_SUCCESS) {
      std::cerr << "Failed to process audio: " << pv_status_to_string(status) << std::endl;
      exit(1);
    }

    if (result >= 0) {
      std::cout << "Wake word detected!" << std::endl;
      play_wav(sound_base_path + "greet1.wav");
      is_wake_word_detected = true;
    }

    if (is_wake_word_detected && vosk_recognizer_accept_waveform(recognizer, (const char*)buffer, FRAME_LENGTH * sizeof(int16_t))) {
      const char* result_json = vosk_recognizer_result(recognizer);
      Json::Value root;
      Json::Reader reader;
      if (reader.parse(result_json, root)) {
        std::string text = root["text"].asString();
        std::cout << "Recognized: " << text << std::endl;
        is_wake_word_detected = false;

        for (const auto& cmd : commands) {
          for (const auto& phrase : cmd.phrases) {
            if (text.find(phrase) != std::string::npos) {
              executeCommand(cmd);
              if (cmd.action != "cli" && !cmd.sounds.empty()) {
                std::string sound_file = getRandomSound(cmd.sounds);
                std::cout << "Playing command sound: " << sound_file << std::endl;
                play_wav(sound_base_path + sound_file + ".wav");
              }
              break;
            }
          }
        }
      }
    }
  }
}

void savePid(const std::string& filePath) {
  std::ofstream pidFile(filePath);
  pidFile << getpid();
  pidFile.close();
}

int main() {
  srand(time(0)); // Инициализация генератора случайных чисел
  std::string commandsDir = "/home/silenth/virtual-assistant/commands";
  std::vector<Command> commands = loadCommandsFromDirectory(commandsDir);

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

  savePid("/home/silenth/virtual-assistant/silence.pid");

  process_audio(capture_handle, porcupine, recognizer, commands);

  vosk_recognizer_free(recognizer);
  vosk_model_free(model);
  snd_pcm_drain(capture_handle);
  snd_pcm_close(capture_handle);
  pv_porcupine_delete(porcupine);

  return 0;
}
