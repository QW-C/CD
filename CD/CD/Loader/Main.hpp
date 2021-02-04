#pragma once

namespace CD {

class Main {
public:
	Main() = default;
	virtual ~Main() = default;

	virtual void run() = 0;
};

}