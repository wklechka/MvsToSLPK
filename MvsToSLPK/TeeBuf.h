#pragma once
#include <iostream>
#include <fstream>
#include <streambuf>

class TeeBuf : public std::streambuf {
public:
	TeeBuf(std::streambuf* sb1, std::streambuf* sb2) : sb1(sb1), sb2(sb2) {}

protected:
	virtual int overflow(int c) override {
		if (c == EOF) {
			return !EOF;
		}
		else {
			int const r1 = sb1->sputc(c);
			int const r2 = sb2->sputc(c);
			return r1 == EOF || r2 == EOF ? EOF : c;
		}
	}

	virtual int sync() override {
		int const r1 = sb1->pubsync();
		int const r2 = sb2->pubsync();
		return r1 == 0 && r2 == 0 ? 0 : -1;
	}

private:
	std::streambuf* sb1;
	std::streambuf* sb2;
};
