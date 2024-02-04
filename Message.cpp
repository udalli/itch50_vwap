#include "Message.h"
#include <cstdint>
#include <ctime>
#include <cwchar>
#include <iomanip>
#include <ios>
#include <iostream>

namespace ITCH
{

inline std::ostream &log_timestamp(std::ostream &ss, const Timestamp_t &timestamp)
{
  std::uint64_t remaining_nano = timestamp;
  const auto    hour           = remaining_nano / (1000000000ULL * 3600);
  remaining_nano -= hour * ((1000000000ULL * 3600));
  const auto min = remaining_nano / (1000000000ULL * 60);
  remaining_nano -= min * ((1000000000ULL * 60));
  const auto sec = remaining_nano / (1000000000ULL);
  remaining_nano -= sec * ((1000000000ULL * 1));
  const std::uint64_t nanosec = remaining_nano;
  ss << std::dec << std::setfill('0');
  ss << std::setw(2) << hour << ":";
  ss << std::setw(2) << min << ":";
  ss << std::setw(2) << sec << ".";
  ss << std::setw(9) << nanosec;
  return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const Message &message)
{
  ss << message.get_length() << "b: ";
  ss << message.get_type() << " | ";
  ss << std::setw(4) << std::hex << message.get_stock_locate() << " | ";
  ss << std::setw(4) << std::hex << message.get_tracking_number() << " | ";
  return log_timestamp(ss, message.get_timestamp());
}

inline std::ostream &operator<<(std::ostream &ss, const SystemMessage &message)
{
  ss << (const Message &)message << " | ";
  ss << message.get_event_type();
  return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const AddOrderMessage &message)
{
  ss << (const Message &)message << " | ";
  ss << message.get_order_reference_number() << " | ";
  ss << message.get_order_type() << " | ";
  ss << message.get_nr_shares() << " | ";
  ss << message.get_stock() << " | ";
  ss << message.get_price();
  return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const AddOrderMPIDAttributionMessage &message)
{
  ss << (const AddOrderMessage &)message << " | ";
  ss << message.get_attribution();
  return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const OrderExecutedMessage &message)
{
  ss << (const Message &)message << " | ";
  ss << message.get_order_reference_number() << " | ";
  ss << message.get_nr_shares() << " | ";
  ss << message.get_match_number();
  return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const OrderExecutedWithPriceMessage &message)
{
  ss << (const OrderExecutedMessage &)message << " | ";
  ss << message.get_printable() << " | ";
  ss << message.get_price();
  return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const OrderReplaceMessage &message)
{
  ss << (const Message &)message << " | ";
  ss << message.get_original_order_reference_number() << " | ";
  ss << message.get_new_order_reference_number() << " | ";
  ss << message.get_nr_shares() << " | ";
  ss << message.get_price();
  return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const TradeMessage &message)
{
  ss << (const Message &)message << " | ";
  ss << message.get_order_reference_number() << " | ";
  ss << message.get_order_type() << " | ";
  ss << message.get_nr_shares() << " | ";
  ss << message.get_stock() << " | ";
  ss << message.get_price() << " | ";
  ss << message.get_match_number();
  return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const BrokenTradeMessage &message)
{
  ss << (const Message &)message << " | ";
  ss << message.get_match_number();
  return ss;
}

inline std::string read_string(const unsigned char *bytes, std::size_t length)
{
  return std::string(reinterpret_cast<const char *>(bytes), length);
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

std::size_t Message::get_length() const
{
  return m_length;
}

MessageType Message::get_type() const
{
  return static_cast<MessageType>(read_1(m_raw_data));
}

StockLocate_t Message::get_stock_locate() const
{
  return static_cast<StockLocate_t>(read_2(m_raw_data + 1));
}

TrackingNumber_t Message::get_tracking_number() const
{
  return static_cast<TrackingNumber_t>(read_2(m_raw_data + 3));
}

Timestamp_t Message::get_timestamp() const
{
  return static_cast<Timestamp_t>(read_6(m_raw_data + 5));
}

SystemEventType SystemMessage::get_event_type() const
{
  return static_cast<SystemEventType>(read_1(m_raw_data + 11));
}

OrderReferenceNumber_t AddOrderMessage::get_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data + 11));
}

OrderType AddOrderMessage::get_order_type() const
{
  return static_cast<OrderType>(read_1(m_raw_data + 19));
}

SharesCount_t AddOrderMessage::get_nr_shares() const
{
  return static_cast<SharesCount_t>(read_4(m_raw_data + 20));
}

Stock_t AddOrderMessage::get_stock() const
{
  return read_string(m_raw_data + 24, 8);
}

Price_t AddOrderMessage::get_price() const
{
  return read_4(m_raw_data + 32) / (10000.0);
}

Attribution_t AddOrderMPIDAttributionMessage::get_attribution() const
{
  return read_string(m_raw_data + 36, 4);
}

OrderReferenceNumber_t OrderExecutedMessage::get_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data + 11));
}

SharesCount_t OrderExecutedMessage::get_nr_shares() const
{
  return static_cast<SharesCount_t>(read_4(m_raw_data + 19));
}

MatchNumber_t OrderExecutedMessage::get_match_number() const
{
  return static_cast<MatchNumber_t>(read_8(m_raw_data + 23));
}

Printable OrderExecutedWithPriceMessage::get_printable() const
{
  return static_cast<Printable>(read_1(m_raw_data + 31));
}

Price_t OrderExecutedWithPriceMessage::get_price() const
{
  return read_4(m_raw_data + 32) / (10000.0);
}

OrderReferenceNumber_t OrderReplaceMessage::get_original_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data + 11));
}

OrderReferenceNumber_t OrderReplaceMessage::get_new_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data + 19));
}

SharesCount_t OrderReplaceMessage::get_nr_shares() const
{
  return static_cast<SharesCount_t>(read_4(m_raw_data + 27));
}

Price_t OrderReplaceMessage::get_price() const
{
  return read_4(m_raw_data + 31) / (10000.0);
}

OrderReferenceNumber_t TradeMessage::get_order_reference_number() const
{
  return static_cast<OrderReferenceNumber_t>(read_8(m_raw_data + 11));
}

OrderType TradeMessage::get_order_type() const
{
  return static_cast<OrderType>(read_1(m_raw_data + 19));
}

SharesCount_t TradeMessage::get_nr_shares() const
{
  return static_cast<SharesCount_t>(read_4(m_raw_data + 20));
}

Stock_t TradeMessage::get_stock() const
{
  return read_string(m_raw_data + 24, 8);
}

Price_t TradeMessage::get_price() const
{
  return read_4(m_raw_data + 32) / (10000.0);
}

MatchNumber_t TradeMessage::get_match_number() const
{
  return static_cast<MatchNumber_t>(read_8(m_raw_data + 36));
}

MatchNumber_t BrokenTradeMessage::get_match_number() const
{
  return static_cast<MatchNumber_t>(read_8(m_raw_data + 11));
}

MessageReader::MessageReader(std::string filename) : m_mapped_file(filename, boost::iostreams::mapped_file::readonly)
{
  //  mapped_file_source -> read only
  m_file_start = (const unsigned char *)m_mapped_file.const_data();
  m_file_end   = (const unsigned char *)m_file_start + m_mapped_file.size();

  if (!m_mapped_file.is_open() || !m_file_start)
  {
    throw "Whoops";
  }
}

bool MessageReader::read(std::size_t length)
{
  return false;
  //  if (m_file_start + length >= m_file_end)
  //  {
  //    return false;
  //  }
  //
  //  return m_ifs && m_ifs.read(reinterpret_cast<char *>(m_buffer), length);
}

bool MessageReader::is_done() const
{
  return (m_file_start + MESSAGE_LENGTH_SIZE) >= m_file_end;
}

bool MessageReader::next(Message &message)
{
  if (m_file_start + MESSAGE_LENGTH_SIZE > m_file_end)
  {
    return false;
  }

  const auto message_size = read_2(m_file_start);
  m_file_start += MESSAGE_LENGTH_SIZE;

  if (m_file_start + message_size > m_file_end)
  {
    return false;
  }

  message = Message{m_file_start, message_size};
  m_file_start += message_size;

  return true;
}

MessageHandler::MessageHandler()
{
  //  m_orders.reserve(10ULL * 1024 * 1024);
  //  m_executions.reserve(10ULL * 1024 * 1024);
}

void MessageHandler::handle_message(const Message &message)
{
  const Timestamp_t report_period = 1000000000UL * 3600;
  const auto        timestamp     = message.get_timestamp();
  if (timestamp >= m_last_report_timestamp + report_period)
  {
    report();
    m_last_report_timestamp += report_period;
  }
  static size_t i = 0;
  ++i;
  if (i % 10000000 == 0)
    std::cout << "Add: " << i << std::endl;

  switch (message.get_type())
  {
  // TODO Get rid of casting?
  case MessageType::SystemEvent:
  {
    const auto &submessage = reinterpret_cast<const SystemMessage &>(message);
    std::cout << submessage << std::endl;
    break;
  }
  case MessageType::AddOrder:
  case MessageType::AddOrderMPIDAttribution:
  {
    const auto &submessage = reinterpret_cast<const AddOrderMessage &>(message);
    auto       &order      = m_orders[submessage.get_order_reference_number()];

    order.m_reference_number = submessage.get_order_reference_number();
    order.m_type             = submessage.get_order_type();
    order.m_nr_shares        = submessage.get_nr_shares();
    order.m_stock            = submessage.get_stock();
    order.m_price            = submessage.get_price();
    break;
  }
  case MessageType::OrderExecuted:
  {
    const auto &submessage = reinterpret_cast<const OrderExecutedMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    const auto &order      = m_orders[ref_num];
    auto       &execution  = m_executions[submessage.get_match_number()];

    execution.m_reference_number = ref_num;
    execution.m_nr_shares        = submessage.get_nr_shares();
    execution.m_match_num        = submessage.get_match_number();
    execution.m_price            = order.m_price;
    break;
  }
  case MessageType::OrderExecutedWithPrice:
  {
    const auto &submessage = reinterpret_cast<const OrderExecutedWithPriceMessage &>(message);
    if (Printable::Yes == submessage.get_printable())
    {
      const auto  ref_num   = submessage.get_order_reference_number();
      const auto &order     = m_orders[ref_num];
      auto       &execution = m_executions[submessage.get_match_number()];

      execution.m_reference_number = ref_num;
      execution.m_nr_shares        = submessage.get_nr_shares();
      execution.m_match_num        = submessage.get_match_number();
      execution.m_price            = submessage.get_price();
    }
    break;
  }
  case MessageType::OrderReplace:
  {
    const auto &submessage = reinterpret_cast<const OrderReplaceMessage &>(message);
    auto       &old_order  = m_orders[submessage.get_original_order_reference_number()];
    auto       &new_order  = m_orders[submessage.get_new_order_reference_number()];

    new_order.m_reference_number = submessage.get_new_order_reference_number();
    new_order.m_type             = old_order.m_type;
    new_order.m_nr_shares        = submessage.get_nr_shares();
    new_order.m_stock            = old_order.m_stock;
    new_order.m_price            = submessage.get_price();

    m_orders.erase(old_order.m_reference_number);
    break;
  }
  case MessageType::Trade:
  {
    const auto &submessage = reinterpret_cast<const TradeMessage &>(message);
    break;
  }
  case MessageType::BrokenTrade:
  {
    const auto &submessage = reinterpret_cast<const BrokenTradeMessage &>(message);
    break;
  }
  case MessageType::OrderCancel:
  case MessageType::OrderDelete:
  default:
    // Unused messages
    break;
  }
}

void MessageHandler::report()
{
}
} // namespace ITCH
