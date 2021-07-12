// Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef OPEN_SPIEL_GAMES_YORKTOWN_H_
#define OPEN_SPIEL_GAMES_YORKTOWN_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/abseil-cpp/absl/container/flat_hash_map.h"
#include "open_spiel/abseil-cpp/absl/algorithm/container.h"
#include "open_spiel/games/yorktown/yorktown_board.h"


namespace open_spiel {
namespace yorktown {

using StandardYorktownBoard = YorktownBoard<10>;

// Constants.
inline constexpr int NumPlayers() { return 2; }
inline constexpr double LossUtility() { return -1.0; }
inline constexpr double DrawUtility() { return 0.0; }
inline constexpr double WinUtility() { return 1.0; }
inline constexpr int BoardSize() { return 10; }

// 9 moves possible in 4 directions = 36 possible destinations
inline constexpr int kNumActionDestinations = 36;
//NumActionDestinations * possible Fields (for simplicity 100)
inline constexpr int kNumDistinctActions = 3600;

// The shape of my observationTensor
inline const std::vector<int>& ObservationTensorShape() {
  static std::vector<int> shape{
      28 /* piece types (12) * colours + empty + lake + unknown*2*/ +
          1 /* side to move */ + 1 /*repetition count*/,
      BoardSize(), BoardSize()};
  return shape;
}

// The shape of my InformationStateTensor
inline const std::vector<int>& InformationStateTensorShape() {
  static std::vector<int> shape{
      28 /* piece types (12) * colours + empty + lake + unknown*2*/ +
          1 /* side to move */ + 1 /*repetition count*/,
      BoardSize(), BoardSize()};
  return shape;
}

class YorktownGame;

// A method to get the player (number) from its color
inline int ColorToPlayer(Color c) {
  if (c == Color::kRed) {
    return 0;
  } else if (c == Color::kBlue) {
    return 1;
  } else if (c == Color::kEmpty) {
    return 2;
  } else {
    SpielFatalError("Unknown color");
  }
}

// Returns the opponent
inline int OtherPlayer(Player player) { return player == Player{0} ? 1 : 0; }

// A getter for the color of a player
Color PlayerToColor(Player p);

// Action encoding (must be changed to support larger boards):
// bits 0-5: from square (0-100)
// bits 6-11: to square (0-100)

// Returns index (0 ... BoardSize*BoardSize-1) of a square
// ({0, 0} ... {BoardSize-1, BoardSize-1}).
inline uint8_t SquareToIndex(const Square& square) {
  return square.y * BoardSize() + square.x;
}

// Returns square ({0, 0} ... {BoardSize-1, BoardSize-1}) from an index
// (0 ... BoardSize*BoardSize-1).
inline Square IndexToSquare(uint8_t index) {
  return Square{static_cast<int8_t>(index % BoardSize()),
                static_cast<int8_t>(index / BoardSize())};
}

// This method encodes each possible move to a specific number 
int EncodeMove(const Square& from_square, int destination_index, int board_size,
               int num_actions_destinations);

// This method casts the given move to an action
Action MoveToAction(const Move& move);

// This method takes an action, returning the starting sqaure as well as the destination
// index. With the methods from chess_common it is possible to calculate the offset
// from the destinationindex
std::pair<Square, int> ActionToDestination(int action, int board_size,
                                           int num_actions_destinations);

// This method casts a given action to a move which can than eb played on the board
Move ActionToMove(const Action& action, const StandardYorktownBoard& board);

class YorktownState : public State {
 public:
  // contructors to generate a State
  YorktownState(std::shared_ptr<const Game> game);
  YorktownState(std::shared_ptr<const Game> game, const std::string& strados3);
  YorktownState(const YorktownState&) = default;

  YorktownState& operator=(const YorktownState&) = default;

  // This methods returns the currentplayer if the game is not over
  Player CurrentPlayer() const override {
    return ColorToPlayer(Board().ToPlay());
  }

  // Thie methods returns a vector of all possible actions at the current State
  std::vector<Action> LegalActions() const override;

  // A small method returning the action in form of a LAN move
  std::string ActionToString(Player player, Action action) const override;

  // This method returns the actual State as a StraDos3 FEN notation string
  std::string ToString() const override;

  // checks if the game is over
  bool IsTerminal() const override {
    return static_cast<bool>(MaybeFinalReturns());
  }

  // TODO
  std::vector<double> Returns() const override;

  // This method returns the current InformationString of the player as a String
  // This is identical to the StraDos3 string of the current board
  std::string InformationStateString(Player player) const override;

  // This method returns the current obervations of a player.
  // At the moment this is identical to the InformationStateString
  std::string ObservationString(Player player) const override;

  // The InformationState and Observationtensors return the current State in form of 
  // planes 
  void InformationStateTensor(Player player,
                              absl::Span<float> values) const override;
  void ObservationTensor(Player player,
                         absl::Span<float> values) const override;
  
  // A method to clone the current state
  std::unique_ptr<State> Clone() const override;

  std::unique_ptr<State> CloneAndRandomizeToState() const;
  // A method to clone the current state
  std::unique_ptr<YorktownState> CloneAndRandomize() const;

  // A method undoing the given action (TODO)
  void UndoAction(Player player, Action action) override;
  
  // Current board.
  StandardYorktownBoard& Board() { return current_board_; }
  const StandardYorktownBoard& Board() const { return current_board_; }

  // Starting board.
  StandardYorktownBoard& StartBoard() { return start_board_; }
  const StandardYorktownBoard& StartBoard() const { return start_board_; }

  // The move history in form of an vector
  std::vector<Move>& MovesHistory() { return moves_history_; }
  const std::vector<Move>& MovesHistory() const { return moves_history_; }

  void DebugString();

  int MoveNumber() const { return Board().Movenumber(); }

 protected:
  // Apply the action to the current board
  void DoApplyAction(Action action_id) override;

 private:
  
  bool IsRepetitionDraw() const;

  // Calculates legal actions and caches them. This is separate from
  // LegalActions() as there are a number of other methods that need the value
  // of LegalActions. This is a separate method as it's called from
  // IsTerminal(), which is also called by LegalActions().
  void MaybeGenerateLegalActions() const;

  // calculates the returns for both players in case the game is over
  absl::optional<std::vector<double>> MaybeFinalReturns() const;

  Player cur_player_;  // Player whose turn it is.

  // We have to store every move made to check for repetitions and to implement
  // undo. We store the current board position as an optimization.
  std::vector<Move> moves_history_;
  // We store the start board for history to support games not starting
  // from the start position.
  StandardYorktownBoard start_board_;
  // We store the current board position as an optimization.
  StandardYorktownBoard current_board_;
  // // RepetitionTable records how many times the given hash exists in the history
  // stack (including the current board).
  // We are already indexing by board hash, so there is no need to hash that
  // hash again, so we use a custom passthrough hasher.
  class PassthroughHash {
   public:
    std::size_t operator()(uint64_t x) const {
      return static_cast<std::size_t>(x);
    }
  };
  using RepetitionTable = absl::flat_hash_map<uint64_t, int, PassthroughHash>;
  RepetitionTable repetitions_;
  mutable absl::optional<std::vector<Action>> cached_legal_actions_;
};

class YorktownGame : public Game {
 public:
  explicit YorktownGame(const GameParameters& params);
  int NumDistinctActions() const override {return yorktown::kNumDistinctActions;}
  
  
  std::unique_ptr<State> NewInitialState(
      const std::string& strados3) const override {
    return absl::make_unique<YorktownState>(shared_from_this(), strados3);
  }
  
  std::unique_ptr<State> NewInitialState() const override {
    return absl::make_unique<YorktownState>(shared_from_this(), "FEBMBEFEEFBGIBHIBEDBGJDDDHCGJGDHDLIFKDDHAA__AA__AAAA__AA__AASTQQNSQPTSUPWPVRPXPURNQONNQSNVPTNQRRTYUP r 0");
  }

  std::unique_ptr<State> DeserializeState(
      const std::string& str) const override;

  int NumPlayers() const override { return yorktown::NumPlayers(); }
  double MinUtility() const override { return LossUtility(); }
  double UtilitySum() const override { return DrawUtility(); }
  double MaxUtility() const override { return WinUtility(); }
  
  std::vector<int> InformationStateTensorShape() const override {
    return yorktown::InformationStateTensorShape();
  }
  
  std::vector<int> ObservationTensorShape() const override {
    return yorktown::ObservationTensorShape();
  }

  int MaxGameLength() const override;

};

}  // namespace yorktown
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_YORKTOWN_H_
