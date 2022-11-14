#pragma once

#include <optional>
#include <span>

#include "../as_bytes.hpp"
#include "../comparison.hpp"
#include "../enum.hpp"
#include "../error.hpp"
#include "../timeout.hpp"
#include "../units.hpp"
#include "interface.hpp"

namespace hal {

/**
 * @brief Discard received bytes until the sequence is found
 *
 */
class skip_past
{
public:
  /**
   * @brief Construct a new skip beyond object
   *
   * @param p_serial - serial port to skip through
   * @param p_sequence - sequence to search for. The lifetime of this data
   * pointed to by this span must outlive this object, or not be used when the
   * lifetime of that data is no longer available.
   * @param p_read_limit - the maximum number read attempts from the port before
   * returning. A value 0 will result in no reads from the serial port.
   */
  skip_past(serial& p_serial,
            std::span<const hal::byte> p_sequence,
            size_t p_read_limit = 32)
    : m_serial(&p_serial)
    , m_sequence(p_sequence)
    , m_read_limit(p_read_limit)
  {
  }

  /**
   * @brief
   *
   * @return result<work_state> - only
   */
  result<work_state> operator()()
  {
    if (m_search_index == m_sequence.size()) {
      return work_state::finished;
    }

    for (size_t read_limit = 0; read_limit < m_read_limit; read_limit++) {
      std::array<hal::byte, 1> buffer;
      auto read_result = HAL_CHECK(m_serial->read(buffer));

      if (read_result.received.size() != buffer.size()) {
        return work_state::in_progress;
      }

      // Check if the next byte received matches the sequence
      if (m_sequence[m_search_index] == read_result.received[0]) {
        m_search_index++;
      } else {  // Otherwise set the search index back to the start.
        m_search_index = 0;
      }

      // Check if the search index is equal to the size of the sequence size
      if (m_search_index == m_sequence.size()) {
        return work_state::finished;
      }
    }

    return work_state::in_progress;
  }

private:
  serial* m_serial;
  std::span<const hal::byte> m_sequence;
  size_t m_search_index = 0;
  size_t m_read_limit;
};

/**
 * @brief Non-blocking callable for reading serial data into a buffer
 *
 */
class read_into
{
public:
  /**
   * @brief Construct a new read_into object
   *
   * @param p_serial - serial port to skip through
   * @param p_buffer - buffer to read data into
   * @param p_read_limit - the maximum number read attempts from the port before
   * returning. A value 0 will result in no reads from the serial port.
   */
  read_into(serial& p_serial,
            std::span<hal::byte> p_buffer,
            size_t p_read_limit = 32)
    : m_serial(&p_serial)
    , m_buffer(p_buffer)
    , m_read_limit(p_read_limit)
  {
  }

  /**
   * @brief
   *
   * @return result<work_state> - only
   */
  result<work_state> operator()()
  {
    for (size_t read_limit = 0; read_limit < m_read_limit; read_limit++) {
      if (m_buffer.empty()) {
        return work_state::finished;
      }

      auto read_result = HAL_CHECK(m_serial->read(m_buffer));
      // Set the m_buffer to the amount of bytes remaining to be read.
      m_buffer = read_result.remaining;

      if (read_result.received.empty()) {
        return work_state::in_progress;
      }
    }

    return work_state::in_progress;
  }

private:
  serial* m_serial;
  std::span<hal::byte> m_buffer;
  size_t m_read_limit;
};
}  // namespace hal