#include "Benchmark.h"
#include "Search.h"
#include "elo.h"
#include <thread>

using namespace::std;

void PerftSuite();
void PrintVersion();
uint64_t PerftDivide(unsigned int depth, Position& position);
uint64_t Perft(unsigned int depth, Position& position);
void Bench();

void RL();

bool TestNetwork(Position& pos1, Position& pos2, int Maxgames, bool earlyExit, std::vector<std::string>& openings);

void RLPlayGame(int startingSide, Position& pos1, Position& pos2, SearchData& data1, SearchData& data2, int  Score[3]);

string version = "7";  

void TestSyzygy();

int main(int argc, char* argv[])
{
	srand(time(NULL));

	PrintVersion();
	tb_init("<empty>");

	unsigned long long init[4] = { 0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL }, length = 4;
	init_by_array64(init, length);

	ZobristInit();
	BBInit();

	string Line;					//to read the command given by the GUI
	cout.setf(ios::unitbuf);		// Make sure that the outputs are sent straight away to the GUI

	//EvaluateDebug();				//uncomment for debug purposes. Must be run in debug mode to work
	//PerftSuite();

	tTable.SetSize(1);

	Position position;
	position.StartingPosition();

	unsigned int ThreadCount = 1;

	if (argc == 2 && strcmp(argv[1], "bench") == 0) { Bench(); return 0; }	//currently only supports bench from command line for openBench integration

	RL();

	while (getline(cin, Line))
	{
		istringstream iss(Line);
		string token;

		iss >> token;

		if (token == "uci")
		{
			cout << "id name Halogen " << version << endl;
			cout << "id author Kieren Pearson" << endl;
			cout << "option name Clear Hash type button" << endl;
			cout << "option name Hash type spin default 2 min 2 max 8192" << endl;
			cout << "option name Threads type spin default 1 min 1 max 8" << endl;
			cout << "option name SyzygyPath type string default <empty>" << endl;
			cout << "uciok" << endl;
		}

		else if (token == "isready") cout << "readyok" << endl;

		else if (token == "ucinewgame")
		{
			position.StartingPosition();
			//pawnHashTable.ResetTable();
			tTable.ResetTable();
		}

		else if (token == "position")
		{
			position.Reset();
			position.StartingPosition();

			iss >> token;

			if (token == "fen")
			{
				vector<string> fen;

				while (iss >> token && token != "moves")
				{
					fen.push_back(token);
				}

				if (!position.InitialiseFromFen(fen)) cout << "BAD FEN" << endl;
				if (token == "moves") while (iss >> token) position.ApplyMove(token);
			}

			if (token == "startpos")
			{
				iss >> token;
				if (token == "moves") while (iss >> token) position.ApplyMove(token);
			}
		}

		else if (token == "go")
		{
			int wtime = 0;
			int btime = 0;
			int winc = 0;
			int binc = 0;
			int searchTime = 0;
			int movestogo = 0;

			while (iss >> token)
			{
				if (token == "wtime")	iss >> wtime;
				else if (token == "btime")	iss >> btime;
				else if (token == "winc")	iss >> winc;
				else if (token == "binc")	iss >> binc;
				else if (token == "movetime") iss >> searchTime;
				else if (token == "infinite") searchTime = 2147483647;
				else if (token == "movestogo") iss >> movestogo;
			}

			int movetime = 0;

			if (searchTime != 0) 
				movetime = searchTime;
			else
			{
				if (movestogo == 0)
				{

					if (position.GetTurn() == WHITE)
						movetime = wtime / 20 + winc;
					else
						movetime = btime / 20 + binc;
				}
				else
				{
					if (position.GetTurn() == WHITE)
						movetime = movestogo <= 1 ? wtime : wtime / (movestogo + 1) * 2;	
					else
						movetime = movestogo <= 1 ? btime : btime / (movestogo + 1) * 2;
				}
			}

			thread searchThread([&] {MultithreadedSearch(position, movetime, ThreadCount); });
			searchThread.detach();
			
		}

		else if (token == "setoption")
		{
			iss >> token; //'name'
			iss >> token; 

			if (token == "Clear") 
			{
				iss >> token;
				if (token == "Hash") 
				{
					//pawnHashTable.ResetTable();
					tTable.ResetTable();
				}
			}

			else if (token == "Hash")
			{
				iss >> token; //'value'
				iss >> token;

				int size = stoi(token);

				if (size < 2)
					cout << "info string Hash size too small" << endl;
				else if (size > 8192)
					cout << "info string Hash size too large" << endl;
				else
				{
					tTable.SetSize(stoi(token) - 1);
					//pawnHashTable.Init(1);
				}
			}

			else if (token == "Threads")
			{
				iss >> token; //'value'
				iss >> token;

				int size = stoi(token);

				if (size < 1)
					cout << "info string thread count too small" << endl;
				else if (size > 8)
					cout << "info string thread count too large" << endl;
				else
				{
					ThreadCount = size;
				}
			}

			else if (token == "SyzygyPath")
			{
				iss >> token; //'value'
				iss >> token;

				tb_init(token.c_str());
				TestSyzygy();
			}
		}

		else if (token == "perft")
		{
			iss >> token;
			PerftDivide(stoi(token), position);
		}

		else if (token == "stop") KeepSearching = false;
		else if (token == "print") position.Print();
		else if (token == "quit") return 0;
		else if (token == "bench") Bench();
		
		else cout << "Unknown command" << endl;
	}

	return 0;
}

void PrintVersion()
{
	cout << "Halogen " << version;

#if defined(_WIN64)
	cout << " x64";

	#if defined(USE_POPCNT)
		cout << " POPCNT";
	#endif 

	#if defined(USE_AVX2)
		cout << " AVX2";
	#endif 

	cout << endl;

#elif defined(_WIN32)
	cout << " x86" << endl;
#else
	cout << " UNKNOWN COMPILATION" << endl;
#endif
}

void TestSyzygy()
{
#ifdef DEBUG
	Position testPosition;
	testPosition.InitialiseFromFen("8/6B1/8/8/B7/8/K1pk4/8 b - - 0 1");
	unsigned int result = tb_probe_wdl(testPosition.GetWhitePieces(), testPosition.GetBlackPieces(),
		testPosition.GetPieceBB(WHITE_KING) | testPosition.GetPieceBB(BLACK_KING),
		testPosition.GetPieceBB(WHITE_QUEEN) | testPosition.GetPieceBB(BLACK_QUEEN),
		testPosition.GetPieceBB(WHITE_ROOK) | testPosition.GetPieceBB(BLACK_ROOK),
		testPosition.GetPieceBB(WHITE_BISHOP) | testPosition.GetPieceBB(BLACK_BISHOP),
		testPosition.GetPieceBB(WHITE_KNIGHT) | testPosition.GetPieceBB(BLACK_KNIGHT),
		testPosition.GetPieceBB(WHITE_PAWN) | testPosition.GetPieceBB(BLACK_PAWN),
		testPosition.GetFiftyMoveCount(),
		testPosition.CanCastleBlackKingside() * TB_CASTLING_k + testPosition.CanCastleBlackQueenside() * TB_CASTLING_q + testPosition.CanCastleWhiteKingside() * TB_CASTLING_K + testPosition.CanCastleWhiteQueenside() * TB_CASTLING_Q,
		testPosition.GetEnPassant() <= SQ_H8 ? testPosition.GetEnPassant() : 0,
		testPosition.GetTurn());
	assert(result == TB_BLESSED_LOSS);

	result = tb_probe_root(testPosition.GetWhitePieces(), testPosition.GetBlackPieces(),
		testPosition.GetPieceBB(WHITE_KING) | testPosition.GetPieceBB(BLACK_KING),
		testPosition.GetPieceBB(WHITE_QUEEN) | testPosition.GetPieceBB(BLACK_QUEEN),
		testPosition.GetPieceBB(WHITE_ROOK) | testPosition.GetPieceBB(BLACK_ROOK),
		testPosition.GetPieceBB(WHITE_BISHOP) | testPosition.GetPieceBB(BLACK_BISHOP),
		testPosition.GetPieceBB(WHITE_KNIGHT) | testPosition.GetPieceBB(BLACK_KNIGHT),
		testPosition.GetPieceBB(WHITE_PAWN) | testPosition.GetPieceBB(BLACK_PAWN),
		testPosition.GetFiftyMoveCount(),
		testPosition.CanCastleBlackKingside() * TB_CASTLING_k + testPosition.CanCastleBlackQueenside() * TB_CASTLING_q + testPosition.CanCastleWhiteKingside() * TB_CASTLING_K + testPosition.CanCastleWhiteQueenside() * TB_CASTLING_Q,
		testPosition.GetEnPassant() <= SQ_H8 ? testPosition.GetEnPassant() : 0,
		testPosition.GetTurn(),
		NULL);

	assert(TB_GET_WDL(result) == TB_BLESSED_LOSS);
	assert(TB_GET_FROM(result) == SQ_C2);
	assert(TB_GET_TO(result) == SQ_C1);
	assert(TB_GET_PROMOTES(result) == TB_PROMOTES_KNIGHT);
#endif 
}

void PerftSuite()
{
	ifstream infile("perftsuite.txt");

	unsigned int Perfts = 0;
	unsigned int Correct = 0;
	double Totalnodes = 0;
	Position position;
	string line;

	clock_t before = clock();
	while (getline(infile, line))
	{
		vector<string> arrayTokens;
		istringstream iss(line);
		arrayTokens.clear();

		do
		{
			string stub;
			iss >> stub;
			arrayTokens.push_back(stub);
		} while (iss);

		position.InitialiseFromFen(line);
		
		uint64_t nodes = Perft((arrayTokens.size() - 7) / 2, position);
		if (nodes == stoull(arrayTokens.at(arrayTokens.size() - 2)))
		{
			cout << "\nCORRECT Perft with depth " << (arrayTokens.size() - 7) / 2 << " = " << nodes << " leaf nodes";
			Correct++;
		}
		else
		{
			cout << "\nINCORRECT Perft with depth " << (arrayTokens.size() - 7) / 2 << " = " << nodes << " leaf nodes";
		}

		Totalnodes += nodes;
		Perfts++;
	}
	clock_t after = clock();

	double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;

	cout << "\n\nCompleted perft with: " << Correct << "/" << Perfts << " correct";
	cout << "\nTotal nodes: " << (Totalnodes) << " in " << (elapsed_ms / 1000) << "s";
	cout << "\nNodes per second: " << static_cast<unsigned int>((Totalnodes / elapsed_ms) * 1000);
}

uint64_t PerftDivide(unsigned int depth, Position& position)
{
	clock_t before = clock();

	uint64_t nodeCount = 0;
	vector<Move> moves;
	LegalMoves(position, moves);

	for (size_t i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves.at(i));
		uint64_t ChildNodeCount = Perft(depth - 1, position);
		position.RevertMove();

		moves.at(i).Print();
		cout << ": " << ChildNodeCount << endl;
		nodeCount += ChildNodeCount;
	}

	clock_t after = clock();
	double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;

	cout << "\nTotal nodes: " << (nodeCount) << " in " << (elapsed_ms / 1000) << "s";
	cout << "\nNodes per second: " << static_cast<unsigned int>((nodeCount / elapsed_ms) * 1000);
	return nodeCount;
}

uint64_t Perft(unsigned int depth, Position& position)
{
	if (depth == 0)
		return 1;	//if perftdivide is called with 1 this is necesary

	uint64_t nodeCount = 0;
	vector<Move> moves;
	LegalMoves(position, moves);

	for (size_t i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves.at(i));
		nodeCount += Perft(depth - 1, position);
		position.RevertMove();
	}

	return nodeCount;
}

void Bench()
{
	Timer timer;
	timer.Start();

	uint64_t nodeCount = 0;
	Position position;

	for (size_t i = 0; i < benchMarkPositions.size(); i++)
	{
		if (!position.InitialiseFromFen(benchMarkPositions[i]))
		{
			cout << "BAD FEN!" << endl;
			break;
		}

		uint64_t nodes = BenchSearch(position, 8);
		nodeCount += nodes;
	}

	cout << nodeCount << " nodes " << int(nodeCount / max(timer.ElapsedMs(), 1) * 1000) << " nps" << endl;
}

void RL()
{
	std::cout << "Starting reinforcement learning" << std::endl;

	Position original;
	Position bestYet;
	srand(time(NULL));

	std::default_random_engine generator;
	std::normal_distribution<double> distribution(0, 0.005);

	std::vector<std::string> Openings;

	std::ifstream file_in("D:\\book.epd");
	std::string line;
	while (std::getline(file_in, line))
	{
		Openings.push_back(line);
	}

	for (int i = 0; i < 10000; i++)
	{
		std::shuffle(std::begin(Openings), std::end(Openings), generator);

		Position next = bestYet;
		next.RandomlyChangeWeights(distribution, generator);
		if (TestNetwork(bestYet, next, 5000, true, Openings))
		{
			//std::cout << "Current best updated\n";
			bestYet.net = next.net;
			bestYet.net.WriteToFile();
		}

		if (i % 10 == 0 && i != 0)
		{
			std::shuffle(std::begin(Openings), std::end(Openings), generator);

			std::cout << "\nScore against original:\n";
			TestNetwork(original, bestYet, 5000, false, Openings);
			std::cout << "\n";
		}
	}
}

bool TestNetwork(Position& pos1, Position& pos2, int Maxgames, bool earlyExit, std::vector<std::string>& openings)
{
	int Score[3] = { 0, 0, 0 };
	int i; 

	for (i = 1; i < Maxgames; i++)
	{
		SearchData data1;
		SearchData data2;
		
		pos1.InitialiseFromFen(openings[i]);
		pos2.InitialiseFromFen(openings[i]);
		//pos1.StartingPosition();
		//pos2.StartingPosition();
		RLPlayGame(1, pos1, pos2, data1, data2, Score);

		pos1.InitialiseFromFen(openings[i]);
		pos2.InitialiseFromFen(openings[i]);
		//pos1.StartingPosition();
		//pos2.StartingPosition();
		RLPlayGame(-1, pos1, pos2, data1, data2, Score);

		if (i % 1 == 0 && i != 0)
		{
			Elo::IntervalEstimate diff = Elo::estimate_rating_difference(Score[0], Score[1], Score[2]);
			std::cout << "Result after " << i * 2 << " games: {" << Score[0] << ", " << Score[1] << ", " << Score[2] << "} (w, d, l) ELO: " << diff.estimate << " (95% " << diff.lower << ", " << diff.upper << ")       \r";

			if ((diff.upper < 0) && (!diff.estimate_infinity) && earlyExit)
				break;
		}
	}

	Elo::IntervalEstimate diff = Elo::estimate_rating_difference(Score[0], Score[1], Score[2]);
	std::cout << "Result after " << i * 2 << " games: {" << Score[0] << ", " << Score[1] << ", " << Score[2] << "} (w, d, l) ELO: " << diff.estimate << " (95% " << diff.lower << ", " << diff.upper << ")" << std::endl;

	return (diff.estimate > 0);
}

void RLPlayGame(int startingSide, Position& pos1, Position& pos2, SearchData& data1, SearchData& data2, int  Score[3])
{
	int color = pos1.GetTurn() ? 1 : -1;

	ThreadSharedData data;

	for (int move = 0; true; move++)
	{
		Position& current = color == startingSide ? pos1 : pos2;
		SearchData& currentData = color == startingSide ? data1 : data2;

		SearchResult result = NegaScout(current, 1, 4, LowINF, HighINF, color, 0, true, currentData);
		
		if (result.GetScore() >= 9900)
		{
			if (color == startingSide)
				Score[2]++;
			else
				Score[0]++;

			break;
		}

		if (result.GetScore() <= -9900)
		{
			if (color == startingSide)
				Score[0]++;
			else
				Score[2]++;

			break;
		}

		if (result.GetScore() == 0 && result.GetMove().IsUninitialized())
		{
			Score[1]++;
			break;
		}

		if (move >= 30 && abs(result.GetScore()) < 0.15 && abs(NegaScout(startingSide ? pos2 : pos1, 1, 4, LowINF, HighINF, color, 0, true, currentData).GetScore()) < 0.15)
		{
			Score[1]++;
			break;
		}

		if (move >= 150)
		{
			Score[1]++;
			break;
		}

		if (result.GetMove().IsUninitialized())
		{
			std::cout << "ERROR\n";
		}

		pos1.ApplyMove(result.GetMove());
		pos2.ApplyMove(result.GetMove());

		//current.Print();
		color = -color;
	}
}
