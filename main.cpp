#include <iostream>

#include <map>

#include <vector>

#include "yaml-cpp/yaml.h"

using namespace std;

// Struct Declarations

// TODO: Labels within blocks
// TODO: Redo if commands with function operators

struct Content;

struct Var {
  string name; // Variable name
  string type; // Variable type
  void * value; // Pointer to the variable value

  /// Constructs a new variable with a name, type, and value.
  Var(string name, string type, void * value):
    name(name),
    type(type),
    value(value) {}
};

struct Address {


  /// Constructs a new address with default values.
  Address():
    block_path(),
    content_path(),
    instr_num(0) {}
  vector < string > block_path; // Path to the current block
  vector < int > content_path; // Path to the current content
  int instr_num; // Current instruction number
};

struct Command {
  string type; // Type of command
  Content * content; // Pointer to content

  /// Constructs a new command with a type and optional content.
  Command(string type, Content * content = nullptr):
    type(type),
    content(content) {}

  virtual void my_virtual() {} // Virtual function for polymorphism
};

struct ChoiceCommand: Command {
  string id; // ID of the choice

  /// Constructs a new choice command with an ID and content.
  ChoiceCommand(string id, Content * content):
    Command("choice", content),
    id(id) {}
};

struct GotoCommand: Command {
  Address new_loc; // New location to go to

  /// Constructs a new goto command with a new location.
  GotoCommand(Address new_loc):
    Command("goto"),
    new_loc(new_loc) {}
};

struct IfCommand: Command {
  string test_var; // Variable to test
  string comp_var; // Variable to compare to
  string _operator; // Operator for comparison
  void * constant; // Constant value for comparison
  string data_type; // Data type of comparison
  Content * content; // Content to execute if condition is true

  /// Constructs a new if command with a condition and content.
  IfCommand(Content * content, string test_var, string comp_var, string _operator, void * constant, string data_type):
    Command("if", content),
    test_var(test_var),
    comp_var(comp_var),
    _operator(_operator),
    constant(constant),
    data_type(data_type) {}
};

struct PrintCommand: Command {
  string msg; // Message to print

  /// Constructs a new print command with a message.
  PrintCommand(string msg):
    Command("print"),
    msg(msg) {}
};

struct Content {
  vector < Command * > instr_list; // List of instructions

  /// Constructs a new content with an optional list of instructions.
  Content(vector < Command * > instr_list = {}):
    instr_list(instr_list) {}
};

struct Block {
  Content * content; // Content of the block
  map < string, Block * > blocks; // Sub-blocks

  /// Constructs a new block with content and optional sub-blocks.
  Block(): content(nullptr), blocks() {}
};

struct Story {
  map < string, Var * > vars; // Variables in the story
  Block * start; // Starting block of the story

  /// Constructs a new story with default values.
  Story(): vars(), start(nullptr) {}
};

// Game Logic

struct Game {
  Story story; // The story being played
  map < string, Address > choices; // Choices available to the player
  Address curr_address; // Current address in the story

  /// Constructs a new game with default values.
  Game(): curr_address() {}

  /// Retrieves the command at the given address.
  Command * get_command(Address addr) {
    Block * curr_block = story.start;
    // Navigate through the block path to find the current block
    while (!addr.block_path.empty()) {
      string next_loc = addr.block_path.back();

      curr_block = curr_block -> blocks[next_loc];

      addr.block_path.pop_back();
    }

    // Access the current content from the block
    Content * curr_content = curr_block -> content;

    // Navigate through the content path to find the current command
    while (!addr.content_path.empty()) {
      int next_loc = addr.content_path.back();

      if (next_loc >= curr_content -> instr_list.size()) {
        return nullptr; // Return null if the next location is out of bounds
      }
      curr_content = curr_content -> instr_list[next_loc] -> content;

      addr.content_path.pop_back();
    }

    // Return the command at the current instruction number, or null if out of bounds
    if (addr.instr_num >= curr_content -> instr_list.size()) {
      return nullptr;
    }
    return curr_content -> instr_list[addr.instr_num];
  }

  /// Processes the player's choice.
  bool input_choice(string choice) {
    // Check if the choice is valid
    if (choices.find(choice) == choices.end()) {
      cout << "No valid choice found with id " << choice << "\n";

      return false;
    }

    // Update the current address to the address associated with the choice
    curr_address = choices[choice];
    return true;
  }

  /// Executes the next step in the game.
  bool step() {
    // Retrieve the current command
    Command * curr_command = get_command(curr_address);

    // Handle null command (end of content or error)
    if (curr_command == nullptr) {
      if (curr_address.content_path.empty()) {
        return false; // No more content, end of game
      }

      // Move to the next instruction after the current content
      curr_address.instr_num = curr_address.content_path.back() + 1;
      curr_address.content_path.pop_back();
      return true;
    }

    // Handle each command type
    if (curr_command -> type == "") {
      // This is mostly something to start the if off haha
      cout << "WARNING: Came across command with no type, skipping...";
    } else if (curr_command -> type == "choice") {
      ChoiceCommand * choice_command = dynamic_cast < ChoiceCommand * > (curr_command);

      if (choices.find(choice_command -> id) != choices.end()) {
        cout << "WARNING: A choice was defined with an already used ID" << "\n";
      }

      // Record the address of the current choice
      Address choice_address = curr_address;
      choice_address.content_path.push_back(choice_address.instr_num);
      choice_address.instr_num = 0;

      choices.insert(make_pair(choice_command -> id, choice_address));
    } else if (curr_command -> type == "goto") {
      GotoCommand * goto_command = dynamic_cast < GotoCommand * > (curr_command);

      curr_address = goto_command -> new_loc; // Update the current address to the new location

      return true;
    } else if (curr_command -> type == "if") {
      IfCommand * if_command = dynamic_cast < IfCommand * > (curr_command);


       
      // Evaluate the condition
      // GPT originally hallucination-refactored the `evaluate_condition` function:
      // bool condition = evaluate_condition(if_command); // Evaluate the condition
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
      // If the condition is true, update the current address to the content of the if command
      if (condition) {
        curr_address.content_path.push_back(curr_address.instr_num);
        curr_address.instr_num = 0;

        return true;
      }
    } else if (curr_command -> type == "print") {
      PrintCommand * print_command = dynamic_cast < PrintCommand * > (curr_command);

      cout << print_command -> msg << "\n"; // Print the message
    } else {
      cout << "WARNING: Unrecognized command, skipping..." << "\n";
    }

    // Move to the next instruction
    curr_address.instr_num += 1;

    return true;
  }
};

/// Visits nodes in the YAML file to construct the story structure.
void visit(YAML::Node node, string context) {
  // Handle top-level nodes
  if (context == "top-level") {
    for (auto it = node.begin(); it != node.end(); ++it) {
      if (get < 0 > ( * it).as < string > () == "blocks") {
        // Recursively visit blocks
        visit(get < 1 > ( * it), "blocks");
      }
    }
  }
  // Handle blocks within the story
  if (context == "blocks") {
    for (auto it = node.begin(); it != node.end(); ++it) {
      if (get < 0 > ( * it).as < string > () == "blocks") {
        // Recursively visit nested blocks
        visit(get < 1 > ( * it), "blocks");
      }
    }
  }

  // Additional parsing of YAML nodes can be added here
  /*if (node.IsMap()) {
      string type_of_node
      for (auto it = node.begin(); it != node.end(); ++it) {
          if (get<1>(it) ==
      }
  }*/
}

int main() {
  // Load the YAML file and parse it
  YAML::Node root = YAML::LoadFile("/Users/kylehess/Documents/html/tunnel/yaml_attempts/tunnel_yaml_3/story-examples/hello_world.yaml");

  visit(root, "top-level");

  // Initialize the story structure
  Story story;
  story.start = new Block();
  story.start -> content = new Content();
  story.start -> content -> instr_list.push_back(new PrintCommand("Hello, World!"));

  // Create additional content and commands
  Content * y = new Content();
  y -> instr_list.push_back(new PrintCommand("You started the game!"));
  Address v;
  v.block_path = {
    "new_block"
  };
  y -> instr_list.push_back(new GotoCommand(v));
  ChoiceCommand * x = new ChoiceCommand("begin", y);
  story.start -> content -> instr_list.push_back(x);

  // Create blocks and set up the story structure
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

  // Initialize the game with the story
  Game game;
  game.story = story;

  // Game loop
  while (true) {
    // Execute steps in the game
    while (game.step()) {}

    // Handle choices after each step
    while (true) {
      cout << "\n" << "Valid choices are..." << "\n";
      // Check if there are any choices left
      if (game.choices.empty()) {
        cout << "Oh, it looks like you have no choices left. That's the end of the story then! Goodbye." << "\n\n";
        return 0; // End the game if no choices are left
      }
      // Display valid choices
      for (auto it = game.choices.begin(); it != game.choices.end(); ++it) {
        cout << get < 0 > ( * it) << "\n";
      }
      cout << "\n" << "Now input your choice!" << "\n";

      string choice;
      cin >> choice; // Get the player's choice
      // Process the choice and clear the choices for the next step
      if (game.input_choice(choice)) {
        game.choices.clear();
        cout << "--------------------------------------------------" << "\n";
        cout << "You chose: " << choice << "\n\n";
        break; // Break out of the loop to process the choice
      }
    }
  }
}
