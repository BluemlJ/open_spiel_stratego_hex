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

#include "open_spiel/games/yorktown.h"
#include "environments/chess_related/board.h"
#include "open_spiel/games/yorktown/yorktown_board.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <utility>

#include "open_spiel/game_parameters.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

// This is based on the chess game of open_spiel

namespace open_spiel {
namespace yorktown {

namespace {
// Default Parameters.

constexpr int kNumRepetitionsToDraw = 5;

// Facts about the game
const GameType kGameType{/*short_name=*/"yorktown",
                         /*long_name=*/"Yorktown",
                         GameType::Dynamics::kSequential,
                         GameType::ChanceMode::kDeterministic,
                         GameType::Information::kImperfectInformation,
                         GameType::Utility::kZeroSum,
                         GameType::RewardModel::kTerminal,
                         /*max_num_players=*/2,
                         /*min_num_players=*/2,
                         /*provides_information_state_string=*/true,
                         /*provides_information_state_tensor=*/true,
                         /*provides_observation_string=*/true,
                         /*provides_observation_tensor=*/true,
                         /*parameter_specification=*/
                         {{"players", GameParameter(NumPlayers())},
                         {"strados3", GameParameter("FEBMBEFEEFBGIBHIBEDBGJDDDHCGJGDHDLIFKDDHAA__AA__AAAA__AA__AASTQQNSQPTSUPWPVRPXPURNQONNQSNVPTNQRRTYUP r 0")}}};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new YorktownGame(params));
}


REGISTER_SPIEL_GAME(kGameType, Factory);

// Adds a plane to the information state vector corresponding to the presence
// and absence of the given piece type and colour at each square.
void AddPieceTypePlane(Color color, Player player, PieceType piece_type,
                       const StandardYorktownBoard& board,
                       absl::Span<float>::iterator& value_it) {
  for (int8_t y = 0; y < BoardSize(); ++y) {
    for (int8_t x = 0; x < BoardSize(); ++x) {
      Piece piece_on_board = board.at(Square{x, y});
      *value_it++ =
          ((piece_on_board.color == color && piece_on_board.type == piece_type)
               ? 1.0
               : 0.0);
    }
  }
}

// Adds a plane to the information state vector corresponding to the presence
// and absence of the given piece type and colour at each square.
void AddProbabilityPieceTypePlane(Color color, Player player, PieceType piece_type,
                       const StandardYorktownBoard& board,
                       absl::Span<float>::iterator& value_it) {
  
  double countInvisible = 0;
  double countMovedInvisible = 0;
  for (int8_t y = 0; y < BoardSize(); ++y) {
    for (int8_t x = 0; x < BoardSize(); ++x) {
      Piece piece_on_board = board.at(Square{x, y});
      if(piece_on_board.color == color){
        if(piece_on_board.isVisible == false){
           countInvisible++;
           if(piece_on_board.hasMoved == true){
             countMovedInvisible++;
           }
        }
      }
    }
  }
  
  
  for (int8_t y = 0; y < BoardSize(); ++y) {
    for (int8_t x = 0; x < BoardSize(); ++x) {
      Piece piece_on_board = board.at(Square{x, y});
      if(piece_on_board.isVisible == true || piece_on_board.color == PlayerToColor(player) || color == PlayerToColor(player)){
        if(piece_on_board.color == color){ 
          if(piece_on_board.type == piece_type) {
            *value_it++ = 1.0; 
          }
          else *value_it++ = 0.0;
        }else *value_it++ = 0.0;
      }else{
          if(piece_type == PieceType::kFlag || piece_type == PieceType::kBomb){
              if(piece_on_board.hasMoved == true) *value_it++ = 0.0;
              else{
                // count moved pieces , count invisible , see capturedpieces/livingpieces
                // (livingBombs-visibleBombs)/(unmoved + invisible Pieces)
                for(int i = 0; i < 12; ++i){
                  if(piece_type == kPieceTypes[i] && color == Color::kRed){
                    *value_it++ = ((float) board.LivingPieces()[i]-board.find(Piece{color, piece_type, true}).size())/((float) countInvisible-countMovedInvisible);
                  }
                  if(piece_type == kPieceTypes[i] && color == Color::kBlue){
                    *value_it++ = ((float) board.LivingPieces()[i+12]-board.find(Piece{color, piece_type, true}).size())/((float) countInvisible-countMovedInvisible);
                  }
                }
              }
              // For the flag its 1/unmoved invisible Pieces
          }else{
             // (livingPieceType-visiblePieceType (if there are more then one of the type))
             // /(invisible Pieces)
             for(int i = 0; i < 12; ++i){
                if(piece_type == kPieceTypes[i] && color == Color::kRed){
                  *value_it++ = ((float) board.LivingPieces()[i]-board.find(Piece{color, piece_type, true}).size())/((float) countInvisible);
                }
                if(piece_type == kPieceTypes[i] && color == Color::kBlue){
                  *value_it++ = ((float) board.LivingPieces()[i+12]-board.find(Piece{color, piece_type, true}).size())/((float) countInvisible);
                }
              }
            }
          }  
        }
      }
    }


// Adds a plane to the information state vector corresponding to the presence
// and absence of the given piece type and colour at each square.
void AddUnknownPlane(Color color, const StandardYorktownBoard& board,
                       absl::Span<float>::iterator& value_it) {
  for (int8_t y = 0; y < BoardSize(); ++y) {
    for (int8_t x = 0; x < BoardSize(); ++x) {
      Piece piece_on_board = board.at(Square{x, y});
      *value_it++ =
          ((piece_on_board.isVisible == false && piece_on_board.color != color)
               ? 1.0
               : 0.0);
    }
  }
}

// Adds a uniform scalar plane scaled with min and max.
template <typename T>
void AddScalarPlane(T val, T min, T max,
                    absl::Span<float>::iterator& value_it) {
  double normalized_val = static_cast<double>(val - min) / (max - min);
  for (int i = 0; i < BoardSize() * BoardSize(); ++i)
    *value_it++ = normalized_val;
}

void AddBinaryPlane(bool val, absl::Span<float>::iterator& value_it) {
  AddScalarPlane<int>(val ? 1 : 0, 0, 1, value_it);
}

}  // namespace

YorktownState::YorktownState(std::shared_ptr<const Game> game)
    : State(game),
      start_board_(MakeDefaultBoard()),
      current_board_(start_board_) {
  repetitions_[current_board_.HashValue()] = 1;
}

YorktownState::YorktownState(std::shared_ptr<const Game> game, const std::string& strados3)
    : State(game) {
  auto maybe_board = StandardYorktownBoard::BoardFromStraDos3(strados3);
  SPIEL_CHECK_TRUE(maybe_board);
  start_board_ = *maybe_board;
  current_board_ = start_board_;
  repetitions_[current_board_.HashValue()] = 1;
}

void YorktownState::DoApplyAction(Action action) {
  Color c = OppColor(PlayerToColor(CurrentPlayer()));
  Move move = ActionToMove(action, Board());
  moves_history_.push_back(move);
  Board().ApplyMove(move);
  ++repetitions_[current_board_.HashValue()];
  cached_legal_actions_.reset();
  //std::cout << Board().DebugString(OppColor(PlayerToColor(CurrentPlayer()))) << std::endl;
}


void YorktownState::MaybeGenerateLegalActions() const {
  if (!cached_legal_actions_) {
    cached_legal_actions_ = std::vector<Action>();
    Board().GenerateLegalMoves([this](const Move& move) -> bool {
      //std::cout << move.ToString() << std::endl;
      //std::cout << move.ToLANMove() << std::endl;
    const int n = MovesHistory().size();
    // avoid loops by skipping moves which appeared two moves and six moves before.
    if( !( n > 7 && move ==  MovesHistory()[n-2] && move == MovesHistory()[n-6]) ){
        cached_legal_actions_->push_back(MoveToAction(move));
    }
    return true;
    });
    absl::c_sort(*cached_legal_actions_);
  }
}

std::vector<Action> YorktownState::LegalActions() const {
  MaybeGenerateLegalActions();
  if (IsTerminal()) return {};
  return *cached_legal_actions_;
}

Color PlayerToColor(Player p) {
  SPIEL_CHECK_NE(p, kInvalidPlayer);
  return static_cast<Color>(p);
}

int EncodeMove(const Square& from_square, int destination_index, int board_size,
               int num_actions_destinations) {
  return (from_square.x * board_size + from_square.y) *
             num_actions_destinations +
         destination_index;
}

int8_t ReflectRank(Color to_play, int board_size, int8_t rank) {
  return to_play == Color::kBlue ? board_size - 1 - rank : rank;
}

// MoveToAction
Action MoveToAction(const Move& move) {
  Color color = move.piece.color;
  // We rotate the move to be from player p's perspective.
  Move player_move(move);

  // Rotate move to be from player p's perspective.
  player_move.from.y = ReflectRank(color, BoardSize(), player_move.from.y);
  player_move.to.y = ReflectRank(color, BoardSize(), player_move.to.y);

  // For each starting square, we enumerate 36 actions:
  // - 9 possible moves per direction (Scout can walk max. 9 into one direction)

  // In total, this results in an upper limit of  100*36 = 3600 indices.
  // Like said, thats an upper limit (eg. only 92 fields are possible (8 lakes) and 
  // not only 16 out of 80 units can walk like this. Further the amount of possible 
  // actions on a board are far less.)
  
  int starting_index =
      EncodeMove(player_move.from, 0, BoardSize(), kNumActionDestinations);
  int8_t x_diff = player_move.to.x - player_move.from.x;
  int8_t y_diff = player_move.to.y - player_move.from.y;
  Offset offset{x_diff, y_diff};
  
  std::array<Offset, 8> tmp;
  // For the normal moves, we simply encode starting and destination square.
  int destination_index =
      chess_common::OffsetToDestinationIndex(offset, tmp, BoardSize());
  SPIEL_CHECK_TRUE(destination_index >= 0 && destination_index < 100);
  return starting_index + destination_index;
  
}

// ActionToDestinations
std::pair<Square, int> ActionToDestination(int action, int board_size,
                                           int num_actions_destinations) {
  const int xy = action / num_actions_destinations;
  SPIEL_CHECK_GE(xy, 0);
  SPIEL_CHECK_LT(xy, board_size * board_size);
  const int8_t x = xy / board_size;
  const int8_t y = xy % board_size;
  const int destination_index = action % num_actions_destinations;
  SPIEL_CHECK_GE(destination_index, 0);
  SPIEL_CHECK_LT(destination_index, num_actions_destinations);
  return {Square{x, y}, destination_index};
}

// ActionToMove
Move ActionToMove(const Action& action, const StandardYorktownBoard& board) {
  SPIEL_CHECK_GE(action, 0);
  SPIEL_CHECK_LT(action, kNumDistinctActions);

  // The encoded action represents an action encoded from color's perspective.
  Color color = board.ToPlay();
  
  auto [from_square, destination_index] =
      ActionToDestination(action, BoardSize(), kNumActionDestinations);
  
  SPIEL_CHECK_LT(destination_index, kNumActionDestinations);

  std::array<Offset, 8> tmp;
  
  Offset offset;
  offset = DestinationIndexToOffset(destination_index, tmp,
                                      BoardSize());

  Square to_square = from_square + offset;

  from_square.y = ReflectRank(color, BoardSize(), from_square.y);
  to_square.y = ReflectRank(color, BoardSize(), to_square.y);

  // This uses the current state to infer the piece type.
  Piece piece = {board.ToPlay(), board.at(from_square).type};

  Move move(from_square, to_square, piece);
  return move;
}

std::string YorktownState::ActionToString(Player player, Action action) const {
  Move move = ActionToMove(action, Board());
  return move.ToLANMove();
}

std::string YorktownState::ToString() const { return Board().ToStraDos3(); 
}


std::vector<double> YorktownState::Returns() const {
  auto maybe_final_returns = MaybeFinalReturns();
  if (maybe_final_returns) {
    return *maybe_final_returns;
  } else {
    return {0.0, 0.0};
  }
}

std::string YorktownState::InformationStateString(Player player) const {
   return Board().ToStraDos3();
}

void YorktownState::InformationStateTensor(Player player,
                                   absl::Span<float> values) const {
 
 
  auto value_it = values.begin();

  // Piece configuration. (without Probability)
  for (const auto& piece_type : kPieceTypes) {
    AddPieceTypePlane(Color::kRed, player, piece_type, Board(), value_it);
    AddPieceTypePlane(Color::kBlue, player, piece_type, Board(), value_it);
  }

  // Piece configuration. (with Probability)
  //for (const auto& piece_type : kPieceTypes) {
  //   AddProbabilityPieceTypePlane(Color::kRed, player, piece_type, Board(), value_it);
  //   AddProbabilityPieceTypePlane(Color::kBlue, player, piece_type, Board(), value_it);
  // }

  AddUnknownPlane(Color::kRed, Board(), value_it);
  AddUnknownPlane(Color::kBlue, Board(), value_it);
  
  AddPieceTypePlane(Color::kEmpty, player, PieceType::kEmpty, Board(), value_it);
  AddPieceTypePlane(Color::kEmpty, player, PieceType::kLake, Board(), value_it);
 
 
  // Side to play.
  AddScalarPlane(ColorToPlayer(Board().ToPlay()), 0, 1, value_it);

  int repetitions;
  const auto entry = repetitions_.find(Board().HashValue());
  if(entry == repetitions_.end()) repetitions = 0;
  else repetitions = entry->second;

  // Num repetitions for the current board.
  AddScalarPlane(repetitions, 1, 5, value_it);
  
  // Why fail?
  //SPIEL_CHECK_EQ(value_it, values.end());
}

std::string YorktownState::ObservationString(Player player) const {
  return Board().ToStraDos3(PlayerToColor(player));
}

void YorktownState::ObservationTensor(Player player,
                                   absl::Span<float> values) const {
  
  auto value_it = values.begin();

  // Piece configuration. (without Probability)
  //for (const auto& piece_type : kPieceTypes) {
  //  AddPieceTypePlane(Color::kRed, player, piece_type, Board(), value_it);
  //  AddPieceTypePlane(Color::kBlue, player, piece_type, Board(), value_it);
  //}

  // Piece configuration. (with Probability)
   for (const auto& piece_type : kPieceTypes) {
     AddProbabilityPieceTypePlane(Color::kRed, player, piece_type, Board(), value_it);
     AddProbabilityPieceTypePlane(Color::kBlue, player, piece_type, Board(), value_it);
   }

  AddUnknownPlane(Color::kRed, Board(), value_it);
  AddUnknownPlane(Color::kBlue, Board(), value_it);
  
  AddPieceTypePlane(Color::kEmpty, player, PieceType::kEmpty, Board(), value_it);
  AddPieceTypePlane(Color::kEmpty, player, PieceType::kLake, Board(), value_it);
 
 
  // Side to play.
  AddScalarPlane(ColorToPlayer(Board().ToPlay()), 0, 1, value_it);

  int repetitions;
  const auto entry = repetitions_.find(Board().HashValue());
  if(entry == repetitions_.end()) repetitions = 0;
  else repetitions = entry->second;

  // Num repetitions for the current board.
  AddScalarPlane(repetitions, 1, 5, value_it);
  // Why fail?
  //SPIEL_CHECK_EQ(value_it, values.end());
}

std::unique_ptr<State> YorktownState::Clone() const {
  return CloneAndRandomizeToState();
  //return std::unique_ptr<State>(new YorktownState(*this));
}

std::unique_ptr<State> YorktownState::CloneAndRandomizeToState() const {
  return std::unique_ptr<State>(new YorktownState(*CloneAndRandomize()));
}

std::unique_ptr<YorktownState> YorktownState::CloneAndRandomize() const {
  auto state = new YorktownState(*this);

  std::vector<PieceType> p;

  /*
  if(OppColor(PlayerToColor(CurrentPlayer())) == Color::kBlue){
    for(int i = 12; i < 24; ++i){
      pieces.push_back(Board().LivingPieces()[i]);
    }
  }else{
    for(int i = 0; i < 12; ++i){
      pieces.push_back(Board().LivingPieces()[i]);
    }
  }
  */
 
  for (int8_t y = 0; y < BoardSize(); ++y) {
    for (int8_t x = 0; x < BoardSize(); ++x) {
      Piece piece_on_board = Board().at(Square{x, y}); 
      if(!Board().IsVisible(Square{x, y}) && piece_on_board.color == OppColor(PlayerToColor(CurrentPlayer()))){
        p.push_back(piece_on_board.type);
      }
    }
  }



  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  auto rng = std::default_random_engine {seed};
  std::shuffle(p.begin(), p.end(), rng);

  // unmoved pieces
  for (int8_t y = 0; y < BoardSize(); ++y) {
    for (int8_t x = 0; x < BoardSize(); ++x) {
      Piece piece_on_board = state->Board().at(Square{x, y});
      if(piece_on_board.isVisible == false){
        if(piece_on_board.color == OppColor(PlayerToColor(CurrentPlayer())) && piece_on_board.hasMoved == false){
          if(!p.empty()){
          state->Board().set_square(Square{x, y}, Piece{piece_on_board.color, p.back(), true});
          p.pop_back();
          }else{
            std::cout << "howwww" << std::endl;
          }
        }
      }
    }
  }

  // the rest
  for (int8_t y = 0; y < BoardSize(); ++y) {
    for (int8_t x = 0; x < BoardSize(); ++x) {
      Piece piece_on_board = state->Board().at(Square{x, y});
      if(piece_on_board.isVisible == false){
        if(piece_on_board.color == OppColor(PlayerToColor(CurrentPlayer()))){
          if(!p.empty()){
          state->Board().set_square(Square{x, y}, Piece{piece_on_board.color, p.back(), true});
          p.pop_back();
          }else{
            std::cout << "howwww" << std::endl;
          }
        }else{
          state->Board().set_square(Square{x, y}, Piece{piece_on_board.color, piece_on_board.type, true});
        }
      }
    }
  }
  return std::unique_ptr<YorktownState>(state);
}

void YorktownState::UndoAction(Player player, Action action) {
  // TODO: Make this fast by storing undoing the action instead or replaying the hole game
  SPIEL_CHECK_GE(moves_history_.size(), 1);
  --repetitions_[current_board_.HashValue()];
  moves_history_.pop_back();
  history_.pop_back();
  current_board_ = start_board_;
  for (const Move& move : moves_history_) {
    current_board_.ApplyMove(move);
  }
}

bool YorktownState::IsRepetitionDraw() const {
  const auto entry = repetitions_.find(Board().HashValue());
  if(entry == repetitions_.end()) return 0;
  return entry->second >= kNumRepetitionsToDraw;
}

void YorktownState::DebugString(){
  std::cout << Board().DebugString() << std::endl;
}


absl::optional<std::vector<double>> YorktownState::MaybeFinalReturns() const {
  
  // RepetitionsDraw
  if (IsRepetitionDraw()) {
    //std::cout << "DRAW";
    //std::cout << Board().DebugString() << std::endl;
    return std::vector<double>{DrawUtility(), DrawUtility()};
  }

  // Check if one player has lost his flag
  if(Board().LivingPieces()[0] != 1){
    //std::cout << "RF Missing";
    //std::cout << Board().DebugString() << std::endl;
    return std::vector<double>{LossUtility(), WinUtility()};
    
  }

  if(Board().LivingPieces()[12] != 1){
    //std::cout << "BF Missing";
    //std::cout << Board().DebugString() << std::endl;
    return std::vector<double>{WinUtility(), LossUtility()};
    
  }

  // Compute and cache the legal actions.
  MaybeGenerateLegalActions();
  SPIEL_CHECK_TRUE(cached_legal_actions_);
  bool have_legal_moves = !cached_legal_actions_->empty();
  

  // If we don't have legal moves we are either stalemated or checkmated,
  // depending on whether we are in check or not.
  if(!have_legal_moves) {
    //std::cout << "NLmoves";
    std::vector<double> returns(NumPlayers());
    auto to_play = ColorToPlayer(Board().ToPlay());
    returns[to_play] = LossUtility();
    returns[OtherPlayer(to_play)] = WinUtility();
    return returns;
    
  }
  if(Board().Movenumber() > 1000){
    std::cout << "MN " << Board().Movenumber() << std::endl;
    return std::vector<double>{DrawUtility(), DrawUtility()};
  }

  //if(Board().HasSufficientMaterial() == false) {
  //  return std::vector<double>{DrawUtility(), DrawUtility()};
  //}

  return std::nullopt;
}

int YorktownGame::MaxGameLength() const {
  // I do not have any clue how to calculate this
  return 3000;
}

//TODO
std::unique_ptr<State> YorktownGame::DeserializeState(const std::string& str) const {
  if (str.length() == 0) {
    return NewInitialState();
  }
  std::vector<std::string> lines = absl::StrSplit(str, '\n');
  std::unique_ptr<State> state = NewInitialState(lines[0]);
  
  for (int i = 1; i < lines.size(); ++i) {
    if (lines[i].empty()) continue;
    Action action = static_cast<Action>(std::stol(lines[i]));
    state->ApplyAction(action);
  }
  return state;
}

YorktownGame::YorktownGame(const GameParameters& params) : Game(kGameType, params) {}


}  // namespace yorktown
}  // namespace open_spiel
