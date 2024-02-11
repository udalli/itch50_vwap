// MIT License
//
// Copyright (c) 2024 Ufuk Dalli
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "Message.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

inline void try_prefetch(const void *addr)
{
#if COMPILER_SUPPORTS_BUILTIN_PREFETCH
  __builtin_prefetch(addr, 0 /* read-only */, 0 /* no temporal locality */); // TODO benchmark locality 3?
#elif COMPILER_SUPPORTS_MM_PREFETCH
#ifdef _MSC_VER
#include <intrin.h>
#else {
#include <xmmintrin.h>
}
#endif
  _mm_prefetch(reinterpret_cast<const char *>(addr), _MM_HINT_NTA);
#else
  (void)addr;
#endif
}

namespace ITCH
{

constexpr Timestamp_t SEC_IN_NANOS  = 1'000'000'000;
constexpr Timestamp_t MIN_IN_NANOS  = 60 * SEC_IN_NANOS;
constexpr Timestamp_t HOUR_IN_NANOS = 60 * MIN_IN_NANOS;

constexpr auto REPORT_PERIOD           = HOUR_IN_NANOS;
constexpr auto PRICE_CONVERSION_FACTOR = 1.0 / 10'000;
constexpr auto MESSAGE_LENGTH_SIZE     = 2u;

// Wraps Timestamp_t just for operator<<(ostream&)
struct Timestamp
{
  Timestamp(Timestamp_t val) : m_val(val)
  {
  }

  const Timestamp_t m_val;
};

inline std::ostream &operator<<(std::ostream &ss, const Timestamp &timestamp)

{
  std::uint64_t remaining_nano = timestamp.m_val;
  const auto    hour           = remaining_nano / HOUR_IN_NANOS;
  remaining_nano -= hour * HOUR_IN_NANOS;
  const auto min = remaining_nano / MIN_IN_NANOS;
  remaining_nano -= min * MIN_IN_NANOS;
  const auto sec = remaining_nano / SEC_IN_NANOS;
  remaining_nano -= sec * SEC_IN_NANOS;
  const std::uint64_t nanosec = remaining_nano;
  ss << std::dec << std::setfill('0');
  ss << std::setw(2) << hour << ":";
  ss << std::setw(2) << min << ":";
  ss << std::setw(2) << sec << ".";
  ss << std::setw(9) << nanosec;
  return ss;
}

// inline std::ostream &operator<<(std::ostream &ss, const Message &message)
//{
//   ss << message.get_length() << "b: ";
//   ss << message.get_type() << " | ";
//   ss << std::setw(4) << std::hex << message.get_stock_locate() << " | ";
//   ss << std::setw(4) << std::hex << message.get_tracking_number() << " | ";
//   ss << Timestamp{message.get_timestamp()};
//   return ss;
// }
//
// inline std::ostream &operator<<(std::ostream &ss, const SystemMessage &message)
//{
//   ss << (const Message &)message << " | ";
//   ss << message.get_event_type();
//   return ss;
// }
//
// inline std::ostream &operator<<(std::ostream &ss, const AddOrderMessage &message)
//{
//   ss << (const Message &)message << " | ";
//   ss << message.get_order_reference_number() << " | ";
//   ss << message.get_order_type() << " | ";
//   ss << message.get_nr_shares() << " | ";
//   ss << message.get_stock() << " | ";
//   ss << message.get_price();
//   return ss;
// }
//
// inline std::ostream &operator<<(std::ostream &ss, const AddOrderMPIDAttributionMessage &message)
//{
//   ss << (const AddOrderMessage &)message << " | ";
//   ss << message.get_attribution();
//   return ss;
// }
//
// inline std::ostream &operator<<(std::ostream &ss, const OrderExecutedMessage &message)
//{
//   ss << (const Message &)message << " | ";
//   ss << message.get_order_reference_number() << " | ";
//   ss << message.get_nr_shares() << " | ";
//   ss << message.get_match_number();
//   return ss;
// }
//
// inline std::ostream &operator<<(std::ostream &ss, const OrderExecutedWithPriceMessage &message)
//{
//   ss << (const OrderExecutedMessage &)message << " | ";
//   ss << message.get_printable() << " | ";
//   ss << message.get_price();
//   return ss;
// }
//
// inline std::ostream &operator<<(std::ostream &ss, const OrderReplaceMessage &message)
//{
//   ss << (const Message &)message << " | ";
//   ss << message.get_original_order_reference_number() << " | ";
//   ss << message.get_new_order_reference_number() << " | ";
//   ss << message.get_nr_shares() << " | ";
//   ss << message.get_price();
//   return ss;
// }
//
// inline std::ostream &operator<<(std::ostream &ss, const TradeMessage &message)
//{
//   ss << (const Message &)message << " | ";
//   ss << message.get_order_reference_number() << " | ";
//   ss << message.get_order_type() << " | ";
//   ss << message.get_nr_shares() << " | ";
//   ss << message.get_stock() << " | ";
//   ss << message.get_price() << " | ";
//   ss << message.get_match_number();
//   return ss;
// }
//
// inline std::ostream &operator<<(std::ostream &ss, const BrokenTradeMessage &message)
//{
//   ss << (const Message &)message << " | ";
//   ss << message.get_match_number();
//   return ss;
// }

inline std::string_view read_string(const unsigned char *bytes, std::size_t length)
{
  return std::string_view(reinterpret_cast<const char *>(bytes), length);
}

inline std::uint8_t read_1(const unsigned char *bytes)
{
  return bytes[0];
}

inline std::uint16_t read_2(const unsigned char *bytes)
{
  return (std::uint16_t)bytes[1] | (std::uint16_t)(bytes[0] << 8);
}

inline std::uint32_t read_4(const unsigned char *bytes)
{
  return (std::uint32_t)bytes[3] | (std::uint32_t)(bytes[2] << 8) | (std::uint32_t)(bytes[1] << 16) |
         (std::uint32_t)(bytes[0] << 24);
}

inline std::uint64_t read_6(const unsigned char *bytes)
{
  const std::uint64_t upper = read_2(bytes);
  const std::uint64_t lower = read_4(bytes + 2);
  return lower | (upper << 32);
}

inline std::uint64_t read_8(const unsigned char *bytes)
{
  const std::uint64_t upper = read_4(bytes);
  const std::uint64_t lower = read_4(bytes + 4);
  return lower | (upper << 32);
}

std::size_t Message::get_offset() const
{
  return m_pos;
}

std::size_t Message::get_length() const
{
  return m_raw_data.size();
}

MessageType Message::get_type() const
{
  return static_cast<MessageType>(read_1(m_raw_data.data()));
}

StockLocate_t Message::get_stock_locate() const
{
  return static_cast<StockLocate_t>(read_2(m_raw_data.data() + 1));
}

TrackingNumber_t Message::get_tracking_number() const
{
  return static_cast<TrackingNumber_t>(read_2(m_raw_data.data() + 3));
}

Timestamp_t Message::get_timestamp() const
{
  return static_cast<Timestamp_t>(read_6(m_raw_data.data() + 5));
}

SystemEventType SystemMessage::get_event_type() const
{
  return static_cast<SystemEventType>(read_1(m_raw_data.data() + 11));
}

OrderReferenceNumber_t AddOrderMessage::get_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data.data() + 11));
}

OrderType AddOrderMessage::get_order_type() const
{
  return static_cast<OrderType>(read_1(m_raw_data.data() + 19));
}

SharesCount_t AddOrderMessage::get_nr_shares() const
{
  return static_cast<SharesCount_t>(read_4(m_raw_data.data() + 20));
}

Stock_t AddOrderMessage::get_stock() const
{
  return read_string(m_raw_data.data() + 24, 8);
}

Price_t AddOrderMessage::get_price() const
{
  return read_4(m_raw_data.data() + 32) * PRICE_CONVERSION_FACTOR;
}

Attribution_t AddOrderMPIDAttributionMessage::get_attribution() const
{
  return read_string(m_raw_data.data() + 36, 4);
}

OrderReferenceNumber_t OrderExecutedMessage::get_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data.data() + 11));
}

SharesCount_t OrderExecutedMessage::get_nr_shares() const
{
  return static_cast<SharesCount_t>(read_4(m_raw_data.data() + 19));
}

MatchNumber_t OrderExecutedMessage::get_match_number() const
{
  return static_cast<MatchNumber_t>(read_8(m_raw_data.data() + 23));
}

Printable OrderExecutedWithPriceMessage::get_printable() const
{
  return static_cast<Printable>(read_1(m_raw_data.data() + 31));
}

Price_t OrderExecutedWithPriceMessage::get_price() const
{
  return read_4(m_raw_data.data() + 32) * PRICE_CONVERSION_FACTOR;
}

OrderReferenceNumber_t OrderReplaceMessage::get_original_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data.data() + 11));
}

OrderReferenceNumber_t OrderReplaceMessage::get_new_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data.data() + 19));
}

SharesCount_t OrderReplaceMessage::get_nr_shares() const
{
  return static_cast<SharesCount_t>(read_4(m_raw_data.data() + 27));
}

Price_t OrderReplaceMessage::get_price() const
{
  return read_4(m_raw_data.data() + 31) * PRICE_CONVERSION_FACTOR;
}

OrderReferenceNumber_t OrderCancelMessage::get_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data.data() + 11));
}

SharesCount_t OrderCancelMessage::get_nr_shares() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data.data() + 19));
}

OrderReferenceNumber_t OrderDeleteMessage::get_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data.data() + 11));
}

OrderReferenceNumber_t TradeMessage::get_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data.data() + 11));
}

OrderType TradeMessage::get_order_type() const
{
  return static_cast<OrderType>(read_1(m_raw_data.data() + 19));
}

SharesCount_t TradeMessage::get_nr_shares() const
{
  return static_cast<SharesCount_t>(read_4(m_raw_data.data() + 20));
}

Stock_t TradeMessage::get_stock() const
{
  return read_string(m_raw_data.data() + 24, 8);
}

Price_t TradeMessage::get_price() const
{
  return read_4(m_raw_data.data() + 32) * PRICE_CONVERSION_FACTOR;
}

MatchNumber_t TradeMessage::get_match_number() const
{
  return static_cast<MatchNumber_t>(read_8(m_raw_data.data() + 36));
}

MatchNumber_t BrokenTradeMessage::get_match_number() const
{
  return static_cast<MatchNumber_t>(read_8(m_raw_data.data() + 11));
}

MessageReader::MessageReader(std::string filename)
  : m_file(filename, boost::iostreams::mapped_file::readonly), m_data((const unsigned char *)(m_file.const_data())),
    m_size(m_file.size())
{
  if (!m_file.is_open() || !m_data || (0 == m_size))
  {
    throw "Failed to open file!";
  }
}

bool MessageReader::next(Message &message)
{
  const auto success = read(message, m_pos);

  if (success)
  {
    m_pos += MESSAGE_LENGTH_SIZE + message.get_length();
  }

  return success;
}

bool MessageReader::read(Message &message, size_t pos) const
{
  if (pos + MESSAGE_LENGTH_SIZE > m_size)
  {
    return false;
  }

  const auto message_size = read_2(m_data + pos);

  // request next message at cache
  try_prefetch(m_data + pos + MESSAGE_LENGTH_SIZE + message_size);

  if (pos + MESSAGE_LENGTH_SIZE + message_size > m_size)
  {
    return false;
  }

  message = {std::span(m_data + pos + MESSAGE_LENGTH_SIZE, message_size), pos};

  return true;
}

MessageHandler::MessageHandler(std::shared_ptr<MessageReader> message_reader) : m_message_reader(message_reader)
{
  // TODO Find optimum initial size
  constexpr auto initial_size = 32 * 1024 * 1024;
  m_orders.reserve(initial_size);
}

MessageHandler::~MessageHandler()
{
  // TODO Is this necessary?
  // report(m_last_report_time + REPORT_PERIOD);
}

void MessageHandler::handle_message(const Message &message)
{
  //  static std::unordered_map<MessageType, size_t> counts;
  //  counts[message.get_type()]++;

  const auto timestamp = message.get_timestamp();
  report(timestamp);

  switch (message.get_type())
  {
  // TODO Get rid of casting?
  case MessageType::SystemEvent:
  {
    static const auto event_logs = std::unordered_map<SystemEventType, std::string>{
      {SystemEventType::StartMessages, "Start of Messages"},
      {SystemEventType::StartSystemHours, "Start of System hours"},
      {SystemEventType::StartMarketHours, "Start of Market hours"},
      {SystemEventType::EndMarketHours, "End of Market hours"},
      {SystemEventType::EndSystemHours, "End of System hours"},
      {SystemEventType::EndMessages, "End of Messages"},
    };

    const auto &submessage = static_cast<const SystemMessage &>(message);
    std::cout << Timestamp{timestamp} << " | " << event_logs.at(submessage.get_event_type()) << std::endl;
    break;
  }
  case MessageType::AddOrder:
  case MessageType::AddOrderMPIDAttribution:
  {
    const auto &submessage = static_cast<const AddOrderMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    m_orders[ref_num]      = message.get_offset();

    break;
  }
  case MessageType::OrderReplace:
  {
    const auto &submessage = static_cast<const OrderReplaceMessage &>(message);
    const auto  iter_order = m_orders.find(submessage.get_original_order_reference_number());

    if (m_orders.end() != iter_order)
    {
      m_orders[submessage.get_new_order_reference_number()] = message.get_offset();
    }
    break;
  }
  case MessageType::OrderDelete:
  {
    // TODO Compare IR with vs w/out
    const auto &submessage = static_cast<const OrderDeleteMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    m_orders.erase(ref_num); // TODO check inst fetch
    break;
  }
  case MessageType::OrderCancel:
  {
    // TODO Check if it is worth to remove an order with zero remanining shares
    break;
  }
  case MessageType::OrderExecuted:
  {
    const auto &submessage = static_cast<const OrderExecutedMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    Order       order{};

    if (construct_order(ref_num, order))
    {
      execute_order(order.m_stock, submessage.get_nr_shares(), order.m_price);
    }

    break;
  }
  case MessageType::OrderExecutedWithPrice:
  {
    const auto &submessage = static_cast<const OrderExecutedWithPriceMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    Order       order{};

    if ((Printable::Yes == submessage.get_printable()) && construct_order(ref_num, order))
    {
      execute_order(order.m_stock, submessage.get_nr_shares(), submessage.get_price());
    }
    break;
  }
  case MessageType::Trade:
  {
    const auto &submessage = static_cast<const TradeMessage &>(message);
    execute_order(submessage.get_stock(), submessage.get_nr_shares(), submessage.get_price());
    break;
  }
  case MessageType::BrokenTrade:
  {
    // Ignored, NQTVITCHspecification: "If a firm is only using the ITCH feed to build a book,
    // however, it may ignore these messages as they have no impact on the current book"
    break;
  }
  default:
    // Unused or unknown eessages
    break;
  }
}

bool MessageHandler::construct_order(OrderReferenceNumber_t ref_num, Order& order) const
{
    Message first_order{};
    Message last_order{};
    auto order_iter = m_orders.find(ref_num);

    if ((m_orders.end() == order_iter) || !m_message_reader->read(last_order, order_iter->second))
    {
        std::cerr << "Failed to construct order (order not found)" << ref_num << std::endl;
  

    first_order = last_order;

    while (MessageType::OrderReplace == first_order.get_type())
    {
        const auto& submessage = static_cast<const OrderReplaceMessage&>(first_order);

        order_iter = m_orders.find(submessage.get_original_order_reference_number());

        if ((m_orders.end() == order_iter) || !m_message_reader->read(first_order, order_iter->second))
        {
            std::cerr << "Failed to construct order (order not found)" << ref_num << std::endl;
            return false;
        }
    }

    const auto first_message_type = first_order.get_type();
    const auto last_message_type = last_order.get_type();

    if ((first_message_type != MessageType::AddOrder) &&
        (first_message_type != MessageType::AddOrderMPIDAttribution))
    {
        std::cerr << "Failed to construct order (unexpected message type) " << first_message_type << std::endl;
        return false;
    }

    const auto& submessage = static_cast<const AddOrderMessage&>(first_order);

    order.m_reference_number = submessage.get_order_reference_number();
    order.m_type = submessage.get_order_type();
    order.m_nr_shares = submessage.get_nr_shares();
    order.m_stock = submessage.get_stock();
    order.m_price = submessage.get_price();

    if (last_message_type == MessageType::OrderReplace)
    {
        const auto& submessage = static_cast<const OrderReplaceMessage&>(last_order);
        order.m_reference_number = submessage.get_new_order_reference_number();
        order.m_nr_shares = submessage.get_nr_shares();
        order.m_price = submessage.get_price();
    }

    return true;
} 

void MessageHandler::execute_order(Stock_t stock, SharesCount_t nr_shares, Price_t price)
{
  auto &stock_info = m_stocks[stock];

  stock_info.volume += nr_shares;
  stock_info.price += nr_shares * price;
}

void MessageHandler::report(const Timestamp_t &current_time)
{
  if (m_stocks.empty() || (current_time < m_last_report_time + REPORT_PERIOD))
  {
    return;
  }

  m_last_report_time = std::max(m_last_report_time, (current_time / REPORT_PERIOD) * REPORT_PERIOD);

  const auto        hour = m_last_report_time / REPORT_PERIOD;
  std::stringstream filename;
  // std::stringstream data;

  filename << "Stock_VWAP_" << std::setw(2) << std::setfill('0') << hour << ".csv";

  std::cout << Timestamp{current_time} << " | Reporting VWAP | " << filename.str() << " | " << m_stocks.size()
            << " stocks" << std::endl;

  // TODO Check I/O time (async?)
  std::ofstream ofs(filename.str());

  ofs << "Stock, VWAP" << std::endl;

  for (const auto &[stock, price_volume] : m_stocks)
  {
    const auto VWAP = ((0.0 == price_volume.volume) ? 0.0 : (price_volume.price / price_volume.volume));

    // TODO Is it worth to write with async I/O?
    ofs << stock << ", " << VWAP << std::endl;
  }
}

} // namespace ITCH
