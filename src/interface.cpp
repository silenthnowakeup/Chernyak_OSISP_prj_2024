#include <ncurses.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <signal.h>
#include <yaml-cpp/yaml.h>


struct Command {
  std::string description;
  std::string action;
  std::string cli_cmd;
  std::vector<std::string> cli_args;
};

std::vector<Command> loadCommandsFromFile(const std::string& filePath);
void executeCommand(const Command& cmd);
void drawMenu(WINDOW* menu_win, int highlight, const std::vector<Command>& commands);
void handleSignal(int signal);

int main() {
  std::vector<Command> commands = loadCommandsFromFile("/home/silenth/virtual-assistant/commands/interface_commands.yaml");

  initscr();
  clear();
  noecho();
  cbreak();
  curs_set(0);

  int startx = 0;
  int starty = 0;
  int width = 80;
  int height = 24;

  WINDOW* menu_win = newwin(height, width, starty, startx);
  keypad(menu_win, TRUE);

  int highlight = 1;
  int choice = 0;
  int c;

  while (true) {
    drawMenu(menu_win, highlight, commands);
    c = wgetch(menu_win);
    switch (c) {
    case KEY_UP:
      if (highlight == 1)
        highlight = commands.size();
      else
        --highlight;
      break;
    case KEY_DOWN:
      if (highlight == commands.size())
        highlight = 1;
      else
        ++highlight;
      break;
    case 10:
      choice = highlight;
      executeCommand(commands[choice - 1]);
      break;
    default:
      break;
    }
    if (c == 'q') {
      break;
    }
  }

  clrtoeol();
  refresh();
  endwin();

  return 0;
}

std::vector<Command> loadCommandsFromFile(const std::string& filePath) {
  std::vector<Command> commands;
  YAML::Node config = YAML::LoadFile(filePath);

  if (!config["list"]) {
    std::cerr << "Invalid YAML format: 'list' key not found" << std::endl;
    return commands;
  }

  for (const auto& cmdNode : config["list"]) {
    Command cmd;
    cmd.description = cmdNode["description"].as<std::string>();
    cmd.action = cmdNode["command"]["action"].as<std::string>();
    if (cmd.action == "cli") {
      cmd.cli_cmd = cmdNode["command"]["cli_cmd"].as<std::string>();
      if (cmdNode["command"]["cli_args"]) {
        for (const auto& arg : cmdNode["command"]["cli_args"]) {
          cmd.cli_args.push_back(arg.as<std::string>());
        }
      }
    }
    commands.push_back(cmd);
  }

  return commands;
}

void executeCommand(const Command& cmd) {
  if (cmd.action == "cli") {
    std::string full_cmd = cmd.cli_cmd;
    for (const auto& arg : cmd.cli_args) {
      full_cmd += " " + arg;
    }
    system(full_cmd.c_str());
  } else if (cmd.action == "start_assistant") {
    system("/home/silenth/virtual-assistant/bin/silence &");
  } else if (cmd.action == "stop_assistant") {
    std::ifstream pidFile("/home/silenth/virtual-assistant/tmp/silence.pid");
    if (pidFile.is_open()) {
      int pid;
      pidFile >> pid;
      kill(pid, SIGTERM);
      pidFile.close();
    }
  }
}

void drawMenu(WINDOW* menu_win, int highlight, const std::vector<Command>& commands) {
  int x = 2, y = 2;
  box(menu_win, 0, 0);
  for (size_t i = 0; i < commands.size(); ++i) {
    if (highlight == i + 1) {
      wattron(menu_win, A_REVERSE);
      mvwprintw(menu_win, y, x, "%s", commands[i].description.c_str());
      wattroff(menu_win, A_REVERSE);
    } else {
      mvwprintw(menu_win, y, x, "%s", commands[i].description.c_str());
    }
    ++y;
  }
  wrefresh(menu_win);
}

void handleSignal(int signal) {
  endwin();
  exit(signal);
}
