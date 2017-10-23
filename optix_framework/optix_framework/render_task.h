#pragma once
#include <string>
#include <memory>

class RenderTask
{
public:
	RenderTask(std::string destination_file, bool close_program_on_exit);
	virtual ~RenderTask() = default;
	virtual void start() = 0;
	virtual void update(float deltaTime) = 0;
	virtual bool is_active() = 0;
	virtual bool is_finished() = 0;
	virtual std::string get_progress_string() = 0;
	virtual bool on_draw();
	virtual void end();
	const std::string& get_destination_file() { return destination_file; }

protected:
	bool close_program_on_exit = false;
	std::string destination_file = "./result.raw";
};

class RenderTaskFrames : public RenderTask
{
public:
	RenderTaskFrames(int destination_samples, const std::string& destination_file, bool close_program_on_exit);
	void start() override;
	void update(float deltaTime) override;
	bool is_active() override;
	bool is_finished() override;
	std::string get_progress_string() override;
	void end() override;
	bool on_draw() override;

private:
	int destination_samples;
	int current_frame;
};

class RenderTaskTime : public RenderTask
{
public:
	RenderTaskTime(float destination_time, const std::string& destination_file, bool close_program_on_exit);
	void start() override;
	void update(float deltaTime) override;
	bool is_active() override;
	bool is_finished() override;
	std::string get_progress_string() override;
	void end() override;
	bool on_draw() override;

private:
	float destination_time = 10.0f;
	float current_time;
};

class RenderTaskTimeorFrames : public RenderTask
{
public:
	RenderTaskTimeorFrames(int max_frames, float destination_time, const std::string& destination_file, bool close_program_on_exit);
	void start() override;
	void update(float deltaTime) override;
	bool is_active() override;
	bool is_finished() override;
	std::string get_progress_string() override;
	void end() override;
	bool on_draw() override;
private:
	float destination_time = 10.0f;
	float current_time;
	int destination_samples;
	int current_frame;
};