/*
 * This file is part of BBP Pairings, a Swiss-system chess tournament engine
 * Copyright (C) 2016  Jeremy Bierema
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3.0, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <iomanip>
#include <iostream>
#include <list>
#include <ostream>
#include <string>

#include <swisssystems/common.h>
#include <utility/uintstringconversion.h>

#include "checker.h"
#include "tournament.h"

namespace tournament
{
  namespace checker
  {
    /**
     * Check the pairings of the provided tournament, and write a report to
     * std::cout.
     */
    void check(
      const Tournament &originalTournament,
      const swisssystems::SwissSystem swissSystem,
      std::ostream *checklistStream,
      const std::string &filename)
    {
      const swisssystems::Info &info = swisssystems::getInfo(swissSystem);
      Tournament tournament = originalTournament;

      for (Player &player : tournament.players)
      {
        player.matches.clear();
        player.scoreWithoutAcceleration = 0;
      }

      // Iterate over the rounds.
      for (
        tournament.playedRounds = 0;
        tournament.playedRounds < originalTournament.playedRounds;
        ++tournament.playedRounds)
      {
        // Add byes.
        for (const player_index playerIndex : tournament.playersByRank)
        {
          Player &player = tournament.players[playerIndex];
          const Match &originalMatch =
            originalTournament.players
              [playerIndex]
              .matches
              [tournament.playedRounds];
          if (!originalMatch.participatedInPairing)
          {
            player.matches.push_back(originalMatch);
          }
        }

        tournament.updateRanks();
        tournament.computePlayerData();

        std::cout << filename
          << ": Round #"
          << utility::uintstringconversion
              ::toString(tournament.playedRounds + 1u)
          << std::endl;
        if (checklistStream)
        {
          *checklistStream << "Round #"
            << utility::uintstringconversion
                 ::toString(tournament.playedRounds + 1u)
            << std::endl;
        }

        std::list<swisssystems::Pairing> correctMatching;

        try
        {
          correctMatching =
            info.computeMatching(
              Tournament(tournament),
              checklistStream);

          // Find the incorrect pairings.
          std::list<swisssystems::Pairing> providedMatching;
          for (
            decltype(correctMatching)::const_iterator iterator =
                correctMatching.begin(),
              nextIterator = iterator;
            iterator != correctMatching.end();
            iterator = nextIterator)
          {
            ++nextIterator;
            const Match &whiteMatch =
              originalTournament.players
                [iterator->white]
                .matches
                [tournament.playedRounds];

            if (whiteMatch.opponent == iterator->black)
            {
              correctMatching.erase(iterator);
            }
            else
            {
              const Match &blackMatch =
                originalTournament.players
                  [iterator->black]
                  .matches
                  [tournament.playedRounds];

              if (iterator->white <= whiteMatch.opponent)
              {
                providedMatching.emplace_back(
                  iterator->white,
                  whiteMatch.opponent,
                  whiteMatch.color);
              }
              if (iterator->black < blackMatch.opponent)
              {
                providedMatching.emplace_back(
                  iterator->black,
                  blackMatch.opponent,
                  blackMatch.color);
              }
            }
          }

          // Output the incorrect pairings.
          if (!correctMatching.empty())
          {
            swisssystems::sortResults(providedMatching, tournament);

            std::cout << "  Checker pairings"
              << std::setfill(' ')
              << std::setw(8)
              << ""
              << "Tournament pairings "
              << std::endl;

            std::list<swisssystems::Pairing>::const_iterator
                providedPairingsIterator =
              providedMatching.begin();
            for (const swisssystems::Pairing &correctPairing : correctMatching)
            {
              std::cout
                << "    "
                << std::right
                << std::setw(3)
                << utility::uintstringconversion
                    ::toString(correctPairing.white + 1u)
                << " - "
                << std::setw(3)
                << (correctPairing.black == correctPairing.white
                      ? "0"
                      : utility::uintstringconversion
                          ::toString(correctPairing.black + 1u))
                << std::setw(16)
                << ""
                << std::setw(3)
                << utility::uintstringconversion::toString(
                    providedPairingsIterator->white + 1u)
                << " - "
                << std::setw(3)
                << (providedPairingsIterator->black
                        == providedPairingsIterator->white
                      ? "0"
                      : utility::uintstringconversion
                          ::toString(providedPairingsIterator->black + 1u))
                << std::endl;

              ++providedPairingsIterator;
            }
            std::cout << std::endl;
          }
        }
        catch (const swisssystems::NoValidPairingException &exception)
        {
          std::cout << "No valid pairing exists for round "
            << utility::uintstringconversion
                ::toString(tournament.playedRounds + 1u)
            << '.'
            << std::endl
            << std::endl;
        }

        // Add the results of the round.
        for (const player_index playerIndex : tournament.playersByRank)
        {
          Player &player = tournament.players[playerIndex];
          const Match &match =
            originalTournament.players
              [playerIndex]
              .matches
              [tournament.playedRounds];
          if (match.participatedInPairing)
          {
            player.matches.push_back(match);
          }
          player.scoreWithoutAcceleration +=
            tournament.getPoints(match.matchScore);
        }
      }
    }
  }
}