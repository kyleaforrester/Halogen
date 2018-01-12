#pragma once
#include "BitBoardDefine.h"
#include "Move.h"
#include <vector>

class BoardParamiters;

extern std::vector<BoardParamiters> PreviousParamiters;

class BoardParamiters
{
public:
	BoardParamiters();
	~BoardParamiters();

	unsigned int GetTurnCount() const { return m_TurnCount; }
	bool GetTurn() const { return m_CurrentTurn; }
	bool HasCastledWhite() const { return m_HasCastledWhite; }
	bool HasCastledBlack() const { return m_HasCastledBlack; }
	bool CanCastleWhiteKingside() const { return m_WhiteKingCastle; }
	bool CanCastleWhiteQueenside() const { return m_WhiteQueenCastle; }
	bool CanCastleBlackKingside() const { return m_BlackKingCastle; }
	bool CanCastleBlackQueenside() const { return m_BlackQueenCastle; }
	unsigned int GetEnPassant() const { return m_EnPassant; }
	unsigned int GetCapturePiece() const { return m_CapturePiece; }
	unsigned int GetPromotionPiece() const { return m_PromotionPiece; }

	void SetEnPassant(unsigned int var) { m_EnPassant = var; }
	void SetCaptureSquare(unsigned int var) { m_CaptureSquare = var; }
	void SetCapturePiece(unsigned int var) { m_CapturePiece = var; }
	void SetPromotionPiece(unsigned int var) { m_CapturePiece = var; }
	void WhiteCastled();
	void BlackCastled();
	void NextTurn();

protected:
	bool InitialiseParamitersFromFen(std::vector<std::string> fen);
	void SaveParamiters();
	void RestorePreviousParamiters();
	void UpdateCastleRights(Move move);

private:
	bool m_CurrentTurn = WHITE;
	bool m_WhiteKingCastle = false;
	bool m_WhiteQueenCastle = false;
	bool m_BlackKingCastle = false;
	bool m_BlackQueenCastle = false;

	unsigned int m_EnPassant = N_SQUARES;
	unsigned int m_FiftyMoveCount = 0;
	unsigned int m_TurnCount = 1;

	unsigned int m_CapturePiece = N_PIECES;
	unsigned int m_CaptureSquare = N_SQUARES;
	unsigned int m_PromotionPiece = N_PIECES;

	bool m_HasCastledWhite = false;
	bool m_HasCastledBlack = false;
};

