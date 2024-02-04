#include <iostream>

#include <map>

#include <vector>

#include "yaml-cpp/yaml.h"

using namespace std;

// TODO: Labels within blocks
// TODO: Redo if commands with function operators

struct Content;

struct Var {
  string name;
  string type;
  void * value;

  Var(string name, string type, void * value):
    name(name),
    type(type),
    value(value) {}
};

struct Address {
  Address():
    block_path(),
    content_path(),
    instr_num(0) {}

  vector < string > block_path;
  vector < int > content_path;
  int instr_num;
};

struct Command {
  string type;
  Content * content;

  Command(string type, Content * content = nullptr):
    type(type),
    content(content) {}

  virtual void my_virtual() {}
};

struct ChoiceCommand: Command {
  string id;

  ChoiceCommand(string id, Content * content):
    Command("choice", content),
    id(id) {}
};

struct GotoCommand: Command {
  Address new_loc;

  GotoCommand(Address new_loc):
    Command("goto"),
    new_loc(new_loc) {}
};

struct IfCommand: Command {
  string test_var;
  string comp_var;
  string _operator;
  void * constant;
  string data_type;
  Content * content;

  IfCommand(Content * content, string test_var, string comp_var, string _operator, void * constant, string data_type):
    Command("if", content),
    test_var(test_var),
    comp_var(comp_var),
    _operator(_operator),
    constant(constant),
    data_type(data_type) {}
};

struct PrintCommand: Command {
  string msg;

  PrintCommand(string msg):
    Command("print"),
    msg(msg) {}
};

struct Content {
  vector < Command * > instr_list;

  Content():
    instr_list() {}

  Content(vector < Command * > instr_list):
    instr_list(instr_list) {}
};

struct Block {
  Content * content;
  map < string, Block * > blocks;
};

struct Story {
  map < string, Var * > vars;
  Block * start;
};

struct Game {
  Story story;
  map < string, Address > choices;
  Address curr_address;

  Game():
    curr_address() {}

  Command * get_command(Address addr) {
    Block * curr_block = story.start;

    while (!addr.block_path.empty()) {
      string next_loc = addr.block_path.back();

      curr_block = curr_block -> blocks[next_loc];

      addr.block_path.pop_back();
    }

    Content * curr_content = curr_block -> content;

    while (!addr.content_path.empty()) {
      int next_loc = addr.content_path.back();

      if (next_loc >= curr_content -> instr_list.size()) {
        return nullptr;
      }
      curr_content = curr_content -> instr_list[next_loc] -> content;

      addr.content_path.pop_back();
    }

    if (addr.instr_num >= curr_content -> instr_list.size()) {
      return nullptr;
    }
    return curr_content -> instr_list[addr.instr_num];
  }

  bool input_choice(string choice) {
    if (choices.find(choice) == choices.end()) {
      cout << "No valid choice found with id " << choice << "\n";

      return false;
    }

    curr_address = choices[choice];
    return true;
  }

  bool step() {
    Command * curr_command = get_command(curr_address);

    if (curr_command == nullptr) {
      if (curr_address.content_path.empty()) {
        return false;
      }

      curr_address.instr_num = curr_address.content_path.back() + 1;
      curr_address.content_path.pop_back();
      return true;
    }

    if (curr_command -> type == "") {
      // This is mostly something to start the if off haha
      cout << "WARNING: Came across command with no type, skipping...";
    } else if (curr_command -> type == "choice") {
      ChoiceCommand * choice_command = dynamic_cast < ChoiceCommand * > (curr_command);

      if (choices.find(choice_command -> id) != choices.end()) {
        cout << "WARNING: A choice was defined with an already used ID" << "\n";
      }

      Address choice_address = curr_address;
      choice_address.content_path.push_back(choice_address.instr_num);
      choice_address.instr_num = 0;

      choices.insert(make_pair(choice_command -> id, choice_address));
    } else if (curr_command -> type == "goto") {
      GotoCommand * goto_command = dynamic_cast < GotoCommand * > (curr_command);

      curr_address = goto_command -> new_loc;

      return true;
    } else if (curr_command -> type == "if") {
      IfCommand * if_command = dynamic_cast < IfCommand * > (curr_command);

      bool condition = false;
      if (if_command -> data_type == "int") {
        int value_1 = * ((int * ) story.vars[if_command -> test_var] -> value);

        int value_2;
        if (if_command -> constant != nullptr) {
          value_2 = * ((int * ) if_command -> constant);
        } else {
          value_2 = * ((int * ) story.vars[if_command -> comp_var] -> value);
        }

        if (if_command -> _operator == "equals") {
          condition = (value_1 == value_2);
        } else if (if_command -> _operator == "not_equals") {
          condition = (value_1 != value_2);
        } else if (if_command -> _operator == "less_than") {
          condition = (value_1 < value_2);
        } else if (if_command -> _operator == "at_most") {
          condition = (value_1 <= value_2);
        } else if (if_command -> _operator == "greater_than") {
          condition = (value_1 > value_2);
        } else if (if_command -> _operator == "at_least") {
          condition = (value_1 >= value_2);
        }
      } else if (if_command -> data_type == "string") {
        string value_1 = * ((string * ) story.vars[if_command -> test_var] -> value);

        string value_2;
        if (if_command -> constant != nullptr) {
          string value_2 = * ((string * ) if_command -> constant);
        } else {
          string value_2 = * ((string * ) story.vars[if_command -> comp_var] -> value);
        }

        if (if_command -> _operator == "equals") {
          condition = (value_1 == value_2);
        }
      } else if (if_command -> data_type == "bool") {
        condition = story.vars[if_command -> test_var] -> value;
      }

      if (condition) {
        curr_address.content_path.push_back(curr_address.instr_num);
        curr_address.instr_num = 0;

        return true;
      }
    } else if (curr_command -> type == "print") {
      PrintCommand * print_command = dynamic_cast < PrintCommand * > (curr_command);

      cout << print_command -> msg << "\n";
    } else {
      cout << "WARNING: Unrecognized command, skipping..." << "\n";
    }

    curr_address.instr_num += 1;

    return true;
  }
};

void visit(YAML::Node node, string context) {
  if (context == "top-level") {
    for (auto it = node.begin(); it != node.end(); ++it) {
      if (get < 0 > ( * it).as < string > () == "blocks") {
        visit(get < 1 > ( * it), "blocks");
      }
    }
  }
  if (context == "blocks") {
    for (auto it = node.begin(); it != node.end(); ++it) {
      if (get < 0 > ( * it).as < string > () == "blocks") {
        visit(get < 1 > ( * it), "blocks");
      }
    }
  }

  /*if (node.IsMap()) {
      string type_of_node
      for (auto it = node.begin(); it != node.end(); ++it) {
          if (get<1>(it) ==
      }
  }*/
}

int main() {
  YAML::Node root = YAML::LoadFile("/Users/kylehess/Documents/html/tunnel/yaml_attempts/tunnel_yaml_3/story-examples/hello_world.yaml");

  visit(root, "top-level");

  Story story;
  story.start = new Block();
  story.start -> content = new Content();
  story.start -> content -> instr_list.push_back(new PrintCommand("Hello, World!"));
  Content * y = new Content();
  y -> instr_list.push_back(new PrintCommand("You started the game!"));
  Address v;
  v.block_path = {
    "new_block"
  };
  y -> instr_list.push_back(new GotoCommand(v));
  ChoiceCommand * x = new ChoiceCommand("begin", y);
  story.start -> content -> instr_list.push_back(x);
  Block * z = new Block();
  Content * w = new Content();
  w -> instr_list.push_back(new PrintCommand("You have reached the end of the game!"));
  w -> instr_list.push_back(new PrintCommand("No, wait, we need to test if commands first!"));
  // Add if command to w
  int value2 = 0;
  int * value2_ptr = & value2;
  story.vars["test_var"] = new Var("test_var", "int", value2_ptr);
  int value = 0;
  int * value_ptr = & value;
  Content * my_if_content = new Content();
  my_if_content -> instr_list.push_back(new PrintCommand("The 'if' worked!"));
  IfCommand * my_if_command = new IfCommand(my_if_content, "test_var", "", "equals", value_ptr, "int");
  w -> instr_list.push_back(my_if_command);
  z -> content = w;
  story.start -> blocks.insert(make_pair("new_block", z));

  Game game;
  game.story = story;

  while (true) {
    while (game.step()) {}

    while (true) {
      cout << "\n" << "Valid choices are..." << "\n";

      if (game.choices.empty()) {
        cout << "Oh, it looks like you have no choices left. That's the end of the story then! Goodbye." << "\n\n";
        return 0;
      }
      for (auto it = game.choices.begin(); it != game.choices.end(); ++it) {
        cout << get < 0 > ( * it) << "\n";
      }
      cout << "\n" << "Now input your choice!" << "\n";

      string choice;
      cin >> choice;
      if (game.input_choice(choice)) {
        game.choices.clear();
        cout << "--------------------------------------------------" << "\n";
        cout << "You chose: " << choice << "\n\n";
        break;
      }
    }
  }
}
