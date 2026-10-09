#pragma once

#include "Map/Grid.h"
#include "Search/PathFinder.h"

#include <memory>

// Holds the latest search and which step is shown
class SearchPlayback
{
private:
	// Latest search
	std::shared_ptr<const SearchResult> result;

	// Steps shown
	int displayStep = 0;

	// Playing
	bool playing = false;
	// Leftover partial steps between frames
	double pendingSteps = 0.0;

public:
	// Runs a new search, shown finished or from step 0
	void Run(const Grid& _grid, const SearchSettings& _settings, bool _playFromStart);
	// Removes the search
	void Clear();

	// Starts or resumes playback
	void Play();
	// Pauses playback
	void Pause();
	// Shows _steps more steps, searching first if needed
	void StepForward(const Grid& _grid, const SearchSettings& _settings, int _steps);
	// Jumps to _step, searching first if needed
	void ShowStep(const Grid& _grid, const SearchSettings& _settings, int _step);
	// Jumps forward (1) or back (-1) a quarter of the search
	void MoveByQuarters(const Grid& _grid, const SearchSettings& _settings, int _quarters);
	// Moves playback forward by _seconds, returns true if the step changed
	bool Advance(double _seconds, double _stepsPerSecond);

	// The search being shown, or nullptr
	const std::shared_ptr<const SearchResult>& Result() const { return result; }
	// Current step
	int DisplayStep() const { return displayStep; }

	bool HasResult() const { return result != nullptr; }
	bool IsPlaying() const { return playing; }
	// Stopped partway through
	bool IsPaused() const;
	// Whole search is shown
	bool IsFinished() const;
	// Search is up to date with the grid
	bool MatchesGrid(const Grid& _grid) const;
};
