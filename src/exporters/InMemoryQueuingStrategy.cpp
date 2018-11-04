#include "di/Injectable.h"
#include "exporters/InMemoryQueuingStrategy.h"

BEEEON_OBJECT_BEGIN(BeeeOn, InMemoryQueuingStrategy)
BEEEON_OBJECT_CASTABLE(QueuingStrategy)
BEEEON_OBJECT_END(BeeeOn, InMemoryQueuingStrategy)

using namespace BeeeOn;
using namespace std;

bool InMemoryQueuingStrategy::empty()
{
	return m_vector.empty();
}

size_t InMemoryQueuingStrategy::size()
{
	return m_vector.size();
}

void InMemoryQueuingStrategy::push(const vector<SensorData> &data)
{
	m_vector.insert(m_vector.end(), data.begin(), data.end());
}

size_t InMemoryQueuingStrategy::peek(vector<SensorData> &data, size_t count)
{
	size_t toPeek = count > m_vector.size() ? m_vector.size() : count;

	data.clear();
	data.insert(data.begin(), m_vector.begin(), m_vector.begin() + toPeek);

	return toPeek;
}

void InMemoryQueuingStrategy::pop(size_t count)
{
	m_vector.erase(m_vector.begin(), m_vector.begin() + count);
}
