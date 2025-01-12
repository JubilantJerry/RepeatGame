#pragma once

#include <memory>
#include <vector>
#include <cassert>
#include "BoardGame.h"
#include "BoardGameAI.h"
#include "SmartVector.h"

enum State
{
	TTMOVE, GENERATE_MOVES, MOVELIST
};

template<class AIType>
class MovePickerBase
{
protected:
	typedef typename AIType::AIGameType GameType;
	MovePickerBase(const AIType& ai, GameType* game) : ai(ai), game(game) {}
	const AIType& ai;
	GameType* game;
	static const int MOVE_NONE = GameType::noMoveValue();
};

template<class AIType, bool useStaticMoveEvaluation>
class MovePickerHandling {};

template<class AIType>
class MovePickerHandling<AIType, true> : public MovePickerBase<AIType>
{
	//Static Move evaluation by sorting moves according to given value
protected:
	typedef typename AIType::AIGameType GameType;
	typename GameType::Moves::iterator moveIter;
	typename GameType::Moves::iterator moveEndIter;
	typename GameType::Moves moves;
	MovePickerHandling(const AIType& ai, GameType* game) : MovePickerBase<AIType>(ai, game) {}
	void initialize(const GameType* game)
	{
		game->moves(moves);
		moveIter = moves.data()->begin();
		moveEndIter = moves.data()->end();
		STATIC_IF(COUNTERMOVE_ENABLED(AIType::AIOptions))
		{
			Move counterMove = this->ai.counterMove[game->mapLastMoveToCounterMoveState()];
			if (counterMove != this->MOVE_NONE)
			{
				for (auto it = moves.data()->begin(); it != moves.data()->end(); ++it)
				{
					if (counterMove == *it)
					{
						*it += 100;
						break;
					}
				}
			}
		}
	}

};

template<class AIType>
class MovePickerHandling<AIType, false> : public MovePickerBase<AIType>
{
protected:
	typedef typename AIType::AIGameType GameType;
	typename GameType::MoveGenerator moveGenerator;
	MovePickerHandling(const AIType& ai, GameType* game) : MovePickerBase<AIType>(ai, game) {}
	void initialize(const GameType* game)
	{
		moveGenerator = game->moveGenerator();
	}

};

template<class AIType>
class MovePicker : public MovePickerHandling<AIType, AIType::AIGameType::STATIC_EVALUATION>
{
public:
	typedef typename AIType::AIGameType GameType;

	MovePicker(const AIType& ai, GameType* game, Move ttMove) : MovePickerHandling<AIType, AIType::AIGameType::STATIC_EVALUATION>(ai, game),
		ttMove(ttMove)
	{
		state = (ttMove == this->MOVE_NONE ? GENERATE_MOVES : TTMOVE);
	}

	~MovePicker() {};
	template<bool o = AIType::AIGameType::STATIC_EVALUATION, std::enable_if_t<o>* = nullptr>
	Move pick_best()
	{
		if (this->moveIter != this->moveEndIter)
		{
			auto begin = this->moveIter++;
			std::swap(*begin, *std::max_element(begin, this->moveEndIter, [](ExtMove a, ExtMove b) { return a.value < b.value; }));
			return begin->move;
		}
		return this->MOVE_NONE;
	}
	
	template<bool o = AIType::AIGameType::STATIC_EVALUATION, std::enable_if_t<!o>* = nullptr>
	constexpr Move pick_best()
	{
		return this->moveGenerator.nextMove();
	}

	Move nextMove()
	{
		Move move;
		switch (state)
		{
		case TTMOVE:
			state = GENERATE_MOVES;
			return ttMove;
		case GENERATE_MOVES:
			this->initialize(this->game);
			state = MOVELIST;
		case MOVELIST:
			while ((move = pick_best()) != this->MOVE_NONE)
			{
				if (move != ttMove)
				{
					return move;
				}
			}
			break;
		}
		return this->MOVE_NONE;
	}

private:
	Move ttMove;
	State state;
};

