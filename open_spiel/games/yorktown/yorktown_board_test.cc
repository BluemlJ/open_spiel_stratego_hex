#include "open_spiel/games/yorktown/yorktown_board.h"

#include <memory>
#include <string>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/tests/basic_tests.h"

namespace open_spiel {
namespace yorktown {
namespace {

void BoardGenerationTests() {
  // Test random default board config
  std::string c = "febMBEFEEfbgiBHIBEDBGjdddHCGJGDHDLIFKDDHaa__aa__aaaa__aa__aastpQNSQPTSUPWPVRPXPURnqONNQSNVPtNQRRTYup r 20";
  auto board = YorktownBoard<10>::BoardFromStraDos3(c);
  auto strDos = board->ToStraDos3();
  std::cout << board->DebugString() << std::endl;
  std::cout << board->DebugStringStraDos() << std::endl;
  std::cout << strDos << std::endl;
  SPIEL_CHECK_EQ(strDos, c);

  std::cout << board->DebugString(Color::kRed) << std::endl;
  std::cout << board->DebugStringStraDos(Color::kRed) << std::endl;

  std::cout << board->DebugString(Color::kBlue) << std::endl;
  std::cout << board->DebugStringStraDos(Color::kBlue) << std::endl;

  // Test empty board
  c = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  board = YorktownBoard<10>::BoardFromStraDos3(c);
  strDos = board->ToStraDos3();
  std::cout << board->DebugString() << std::endl;
  std::cout << board->DebugStringStraDos() << std::endl;
  std::cout << strDos << std::endl;
  SPIEL_CHECK_EQ(strDos, c);

  // Test invalid board config
  c = "a r 20";
  SPIEL_CHECK_EQ(YorktownBoard<10>::BoardFromStraDos3(c), std::nullopt);

  // Test invalid board config
  c = "Zaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  SPIEL_CHECK_EQ(YorktownBoard<10>::BoardFromStraDos3(c), std::nullopt);

}

void MoveTests() {
  std::string c = "Caaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  std::string d = "aaaaaaaaaaCaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 21";
  
  // Test LAN move (Piece moves, current_player switches and movenumber increases)
  auto board = MakeDefaultBoard(c);
  absl::optional<Move> maybe_move = board.ParseLANMove("A1A2");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);

  std::cout << board.DebugString() << std::endl;
  std::cout << maybe_move->ToLANMove() << std::endl;
  std::cout << maybe_move->ToStraDosMove() << std::endl;
  std::cout << maybe_move->ToComplexLANMove(board) << std::endl;
  
  // Test StraDos move (Piece moves, current_player switches and movenumber increases)
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseStraDosMove("<move id=\"20\" source=\"A1\" target=\"A2\"/>");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);
  
  // Test ParseMove (Piece moves, current_player switches and movenumber increases)
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("A1A2");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);
  
  // Test StraDos move (Piece moves, current_player switches and movenumber increases)
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("<move id=\"20\" source=\"A1\" target=\"A2\"/>");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);

  // Test illegal move
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseLANMove("A1A3");
  SPIEL_CHECK_FALSE(maybe_move);
  maybe_move = board.ParseStraDosMove("<move id=\"20\" source=\"A1\" target=\"A3\"/>");
  SPIEL_CHECK_FALSE(maybe_move);
  maybe_move = board.ParseMove("A1A3");
  SPIEL_CHECK_FALSE(maybe_move);
  
  // Test Scout
  c = "aaPaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 20";
  d = "aaaaaaaaaaaaPaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 21";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("C1C2");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);
  //std::cout << board->DebugString() << std::endl;

  // Test if lakes block way
  c = "aaPaaaaaaaaaaaaaaaaaaaaaaaaaaaaa_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 20";
  d = "aaaaaaaaaPaaaaaaaaaaaaaaaaaaaaaa_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 21";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("C1C7");
  SPIEL_CHECK_FALSE(maybe_move);
  maybe_move = board.ParseMove("C1K1");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);

  c = "Paaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 20";
  d = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaPaaaaaaaaa r 21";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("A1A:");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);
  
}

void AttackTests() {
  std::string c = "COaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  std::string d = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 21";
  
  // Test LAN move (Piece moves, current_player switches and movenumber increases)
  auto board = MakeDefaultBoard(c);
  absl::optional<Move> maybe_move = board.ParseLANMove("A1B1");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);


  c = "CPaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  d = "apaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 21";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("A1B1");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);

  c = "CXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  d = "acaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 21";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("A1B1");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);
  
  c = "CNaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  d = "anaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 21";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("A1B1");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);

  std::cout << board.DebugString() << std::endl;
  std::cout << maybe_move->ToLANMove() << std::endl;
  std::cout << maybe_move->ToStraDosMove() << std::endl;
  std::cout << maybe_move->ToComplexLANMove(board) << std::endl;
  
  c = "ENaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  d = "aeaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 21";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("A1B1");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);
  
  c = "HUaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  d = "auaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa b 21";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("A1B1");
  SPIEL_CHECK_TRUE(maybe_move);
  board.ApplyMove(*maybe_move);
  SPIEL_CHECK_EQ(board.ToStraDos3(), d);

  c = "CBaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa r 20";
  board = MakeDefaultBoard(c);
  maybe_move = board.ParseMove("A1B1");
  SPIEL_CHECK_FALSE(maybe_move);
  
}

}
}
}

int main(int argc, char** argv) {
  open_spiel::yorktown::BoardGenerationTests();
  open_spiel::yorktown::MoveTests();
  open_spiel::yorktown::AttackTests();
}