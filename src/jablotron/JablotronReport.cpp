#include <Poco/Exception.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/StringTokenizer.h>

#include "jablotron/JablotronReport.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static const unsigned BATTERY_HIGH = 100;
static const unsigned BATTERY_LOW  = 5;

JablotronReport::operator bool() const
{
	return address != 0;
}

bool JablotronReport::operator !() const
{
	return address == 0;
}

static vector<string> tokenize(const string &input)
{
	StringTokenizer tokens(input, " ",
		StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);

	return {tokens.begin(), tokens.end()};
}

bool JablotronReport::has(const string &keyword, bool hasValue) const
{
	for (const auto &token : tokenize(data)) {
		if (hasValue && token.find(keyword + ":") == 0)
			return true;
		if (!hasValue && token == keyword)
			return true;
	}

	return false;
}

int JablotronReport::get(const string &keyword) const
{
	const RegularExpression pattern("^" + keyword + ":([01])$");

	for (const auto &token : tokenize(data)) {
		RegularExpression::MatchVec m;

		if (!pattern.match(token, 0, m))
			continue;

		const auto value = token.substr(m[1].offset, m[1].length);
		return NumberParser::parse(value);
	}

	throw NotFoundException("no value " + keyword);
}

double JablotronReport::temperature(const string &keyword) const
{
	static const string C = {static_cast<char>(0xb0), 'C', '\0'};
	const RegularExpression pattern("^" + keyword + ":(-?[0-9][0-9]\\.[0-9])" + C + "$");

	for (const auto &token : tokenize(data)) {
		RegularExpression::MatchVec m;

		if (!pattern.match(token, 0, m))
			continue;

		const auto value = token.substr(m[1].offset, m[1].length);
		return NumberParser::parseFloat(value);
	}

	throw NotFoundException("no value " + keyword);
}

unsigned int JablotronReport::battery() const
{
	return get("LB") ? BATTERY_LOW : BATTERY_HIGH;
}

string JablotronReport::toString() const
{
	return "[" + NumberFormatter::format0(address, 8) + "] "
		+ type + " " + data;
}

JablotronReport JablotronReport::invalid()
{
	return {0, "", ""};
}
