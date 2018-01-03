#pragma once

#include <chrono>
#include <numeric>
#include <vector>

namespace qvr
{

class Profiler {
public:
	using SampleUnit = std::chrono::duration<float, std::milli>;

	void AddSample(SampleUnit sample) {
		int index = (mFront++) % samples.size();
		samples[index] = sample;
	}

	int BufferSize() const { return samples.size(); }

	SampleUnit GetSample(const int index) const {
		return samples[index];
	}

	int GetFrontIndex() const {
		return mFront;
	}

	SampleUnit GetAverage() const {
		const auto total =
			std::accumulate(
				std::begin(samples),
				std::end(samples),
				SampleUnit(0));
		return total / samples.size();
	}

	Profiler(const unsigned sampleCount) : mFront(0) {
		samples.resize(sampleCount);
	}

	void Resize(const unsigned newSampleCount) {
		mFront = 0;
		samples.resize(newSampleCount);
	}

private:
	// Rolling buffer of samples.
	std::vector<SampleUnit> samples;

	int mFront = 0;
};

class ProfilerScope
{
	Profiler& profiler;
	
	std::chrono::time_point<std::chrono::steady_clock> start;
	
	auto Now() {
		return std::chrono::high_resolution_clock::now();
	}

public:

	ProfilerScope(Profiler& profiler)
		: profiler(profiler)
		, start(Now())
	{}

	~ProfilerScope() {
		profiler.AddSample(
			std::chrono::duration_cast<Profiler::SampleUnit>(Now() - start));
	}
};

}