#include "Message.h"
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <cwchar>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string_view>

namespace ITCH
{

constexpr auto SEC_IN_NANOS  = 1'000'000'000UL;
constexpr auto MIN_IN_NANOS  = 60 * SEC_IN_NANOS;
constexpr auto HOUR_IN_NANOS = 60 * MIN_IN_NANOS;

constexpr auto REPORT_PERIOD           = HOUR_IN_NANOS;
constexpr auto PRICE_CONVERSION_FACTOR = 10'000.0;
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

inline std::ostream &operator<<(std::ostream &ss, const Message &message)
{
  ss << message.get_length() << "b: ";
  ss << message.get_type() << " | ";
  ss << std::setw(4) << std::hex << message.get_stock_locate() << " | ";
  ss << std::setw(4) << std::hex << message.get_tracking_number() << " | ";
  ss << Timestamp{message.get_timestamp()};
  return ss;
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
  return read_4(m_raw_data.data() + 32) / PRICE_CONVERSION_FACTOR;
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
  return read_4(m_raw_data.data() + 32) / PRICE_CONVERSION_FACTOR;
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
  return read_4(m_raw_data.data() + 31) / PRICE_CONVERSION_FACTOR;
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
  return read_4(m_raw_data.data() + 32) / PRICE_CONVERSION_FACTOR;
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
  : m_mapped_file(filename, boost::iostreams::mapped_file::readonly),
    m_data((const unsigned char *)m_mapped_file.const_data(), m_mapped_file.size())
{
  if (!m_mapped_file.is_open() || !m_data.data() || m_data.empty())
  {
    throw "Failed to open file!";
  }
}

bool MessageReader::is_done() const
{
  return (m_pos + MESSAGE_LENGTH_SIZE) >= m_data.size();
}

bool MessageReader::next(Message &message)
{
  if (m_pos + MESSAGE_LENGTH_SIZE > m_data.size())
  {
    return false;
  }

  const auto message_size = read_2(m_data.data() + m_pos);
  m_pos += MESSAGE_LENGTH_SIZE;

  if (m_pos + message_size > m_data.size())
  {
    return false;
  }

  message = Message{m_data.subspan(m_pos, message_size)};
  m_pos += message_size;

  return true;
}

MessageHandler::MessageHandler()
{
  //  m_orders.reserve(10ULL * 1024 * 1024);
  //  m_executions.reserve(10ULL * 1024 * 1024);
}

MessageHandler::~MessageHandler()
{
  // TODO Make it nicer
  report(m_last_report_timestamp + REPORT_PERIOD);
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

    const auto &submessage = reinterpret_cast<const SystemMessage &>(message);
    std::cout << Timestamp{timestamp} << " | " << event_logs.at(submessage.get_event_type()) << std::endl;
    break;
  }
  case MessageType::AddOrder:
  case MessageType::AddOrderMPIDAttribution:
  {
    const auto &submessage = reinterpret_cast<const AddOrderMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    auto       &order      = m_orders[ref_num];

    order.m_reference_number = ref_num;
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
    auto        iter_order = m_orders.find(ref_num);

    if (m_orders.end() != iter_order)
    {
      auto &order     = iter_order->second;
      auto &execution = m_executions[submessage.get_match_number()];

      execution.m_reference_number = ref_num;
      execution.m_type             = order.m_type;
      execution.m_nr_shares        = submessage.get_nr_shares();
      execution.m_stock            = order.m_stock;
      execution.m_match_num        = submessage.get_match_number();
      execution.m_price            = order.m_price;

      if (execution.m_nr_shares >= order.m_nr_shares)
      {
        m_orders.erase(iter_order);
      }
      else
      {
        order.m_nr_shares -= execution.m_nr_shares;
      }

      execute_order(execution, timestamp);
    }

    break;
  }
  case MessageType::OrderExecutedWithPrice:
  {
    const auto &submessage = reinterpret_cast<const OrderExecutedWithPriceMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    auto        iter_order = m_orders.find(ref_num);

    if (Printable::Yes == submessage.get_printable() && m_orders.end() != iter_order)
    {
      auto &order     = iter_order->second;
      auto &execution = m_executions[submessage.get_match_number()];

      execution.m_reference_number = ref_num;
      execution.m_type             = order.m_type;
      execution.m_nr_shares        = submessage.get_nr_shares();
      execution.m_stock            = order.m_stock;
      execution.m_match_num        = submessage.get_match_number();
      execution.m_price            = submessage.get_price();

      if (execution.m_nr_shares >= order.m_nr_shares)
      {
        m_orders.erase(iter_order);
      }
      else
      {
        order.m_nr_shares -= execution.m_nr_shares;
      }

      execute_order(execution, timestamp);
    }
    break;
  }
  case MessageType::OrderReplace:
  {
    const auto &submessage = reinterpret_cast<const OrderReplaceMessage &>(message);
    auto        iter_order = m_orders.find(submessage.get_original_order_reference_number());

    if (m_orders.end() != iter_order)
    {
      auto &old_order              = iter_order->second;
      auto &new_order              = m_orders[submessage.get_new_order_reference_number()];
      new_order.m_reference_number = submessage.get_new_order_reference_number();
      new_order.m_type             = old_order.m_type;
      new_order.m_nr_shares        = submessage.get_nr_shares();
      new_order.m_stock            = old_order.m_stock;
      new_order.m_price            = submessage.get_price();

      m_orders.erase(iter_order);
    }
    break;
  }
  case MessageType::Trade:
  {
    const auto &submessage = reinterpret_cast<const TradeMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    auto       &execution  = m_executions[submessage.get_match_number()];

    execution.m_reference_number = ref_num;
    execution.m_type             = submessage.get_order_type();
    execution.m_nr_shares        = submessage.get_nr_shares();
    execution.m_stock            = submessage.get_stock();
    execution.m_price            = submessage.get_price();
    execution.m_match_num        = submessage.get_match_number();

    execute_order(execution, timestamp);

    break;
  }
  case MessageType::BrokenTrade:
  {
    // Ignored, NQTVITCHspecification: "If a firm is only using the ITCH feed to build a book,
    // however, it may ignore these messages as they have no impact on the current book"
    break;
  }
  case MessageType::OrderCancel:
  {
    const auto &submessage = reinterpret_cast<const OrderCancelMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    auto        iter_order = m_orders.find(ref_num);

    if (m_orders.end() != iter_order)
    {
      auto &order = iter_order->second;

      if (submessage.get_nr_shares() >= order.m_nr_shares)
      {
        m_orders.erase(iter_order);
      }
      else
      {
        order.m_nr_shares -= submessage.get_nr_shares();
      }
    }
    break;
  }
  case MessageType::OrderDelete:
  {
    const auto &submessage = reinterpret_cast<const OrderDeleteMessage &>(message);
    const auto  ref_num    = submessage.get_order_reference_number();
    m_orders.erase(ref_num);
    break;
  }
  default:
    // Unused or unknown messages
    break;
  }
}

void MessageHandler::execute_order(const Execution &execution, const Timestamp_t &timestamp)
{
  auto &stock = m_stocks[execution.m_stock];
  stock.volume += execution.m_nr_shares;
  stock.price += execution.m_nr_shares * execution.m_price;
}

void MessageHandler::report(const Timestamp_t &timestamp)
{
  if (m_stocks.empty() || (timestamp < m_last_report_timestamp + REPORT_PERIOD))
  {
    return;
  }

  m_last_report_timestamp = std::max(m_last_report_timestamp, (timestamp / REPORT_PERIOD) * REPORT_PERIOD);

  const auto reportHour = m_last_report_timestamp / REPORT_PERIOD;
  std::cout << Timestamp{timestamp} << " | dumping " << m_stocks.size() << " stocks" << std::endl;

  std::ofstream ofs(std::to_string(reportHour) + ".csv");
  ofs << "Stock, VWAP" << std::endl;

  for (const auto &[stock, price_volume] : m_stocks)
  {
    const auto VWAP = ((0.0 == price_volume.volume) ? 0.0 : (price_volume.price / price_volume.volume));

    ofs << stock << ", " << VWAP << std::endl;
  }
}

} // namespace ITCH
