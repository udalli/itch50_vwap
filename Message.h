#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "btree/map.h"

namespace ITCH
{

enum MessageType : char
{
  SystemEvent                                 = 'S',
  StockDirectory                              = 'R',
  StockTradingAction                          = 'H',
  RegSHORestriction                           = 'Y',
  MarketParticipantPosition                   = 'L',
  MWCBDeclineLevel                            = 'V',
  MWCBStatus                                  = 'W',
  IPQQuotingPeriodUpdate                      = 'K',
  LULDAuctionCollar                           = 'J',
  OperationalHalt                             = 'h',
  AddOrder                                    = 'A',
  AddOrderMPIDAttribution                     = 'F',
  OrderExecuted                               = 'E',
  OrderExecutedWithPrice                      = 'C',
  OrderCancel                                 = 'X',
  OrderDelete                                 = 'D',
  OrderReplace                                = 'U',
  Trade                                       = 'P',
  CrossTrade                                  = 'Q',
  BrokenTrade                                 = 'B',
  NetOrderImbalanceIndicator                  = 'I',
  RetailInterest                              = 'N', // TODO Is this message valid?
  DirectListingWithCapitalRaisePriceDiscovery = 'O',
};

enum SystemEventType : char
{
  StartMessages    = 'O',
  StartSystemHours = 'S',
  StartMarketHours = 'Q',
  EndMarketHours   = 'M',
  EndSystemHours   = 'E',
  EndMessages      = 'C',
};

enum OrderType : char
{
  Buy  = 'B',
  Sell = 'S',
};

enum Printable : char
{
  Yes = 'Y',
  No  = 'N',
};

using Attribution_t          = std::string;
using MatchNumber_t          = std::uint64_t;
using OrderReferenceNumber_t = std::uint64_t;
using Price_t                = double;
using SharesCount_t          = std::uint32_t;
using StockLocate_t          = std::uint16_t;
using Stock_t                = std::string;   // TODO Slow?
using Timestamp_t            = std::uint64_t; // 48-bit
using TrackingNumber_t       = std::uint16_t;

class Message
{
public:
  Message(const unsigned char *raw_data, std::size_t length) : m_raw_data(raw_data), m_length(length)
  {
  }

  //  virtual ~Message()                  = default;
  //  Message(const Message &)            = default;
  //  Message &operator=(const Message &) = default;
  //  Message(Message &&)                 = default;
  //  Message &operator=(Message &&)      = default;

  std::size_t      get_length() const;
  MessageType      get_type() const;
  StockLocate_t    get_stock_locate() const;
  TrackingNumber_t get_tracking_number() const;
  Timestamp_t      get_timestamp() const;

protected:
  // TODO Use std::span instead
  const unsigned char *m_raw_data;
  std::size_t          m_length;
};

class SystemMessage : public Message
{
public:
  SystemEventType get_event_type() const;
};

class AddOrderMessage : public Message
{
public:
  OrderReferenceNumber_t get_order_reference_number() const;
  OrderType              get_order_type() const;
  SharesCount_t          get_nr_shares() const;
  Stock_t                get_stock() const;
  Price_t                get_price() const;
};

class AddOrderMPIDAttributionMessage : public AddOrderMessage
{
public:
  Attribution_t get_attribution() const;
};

class OrderExecutedMessage : public Message
{
public:
  OrderReferenceNumber_t get_order_reference_number() const;
  SharesCount_t          get_nr_shares() const;
  MatchNumber_t          get_match_number() const;
};

class OrderExecutedWithPriceMessage : public OrderExecutedMessage
{
public:
  Printable get_printable() const;
  Price_t   get_price() const;
};

class OrderReplaceMessage : public Message
{
public:
  OrderReferenceNumber_t get_original_order_reference_number() const;
  OrderReferenceNumber_t get_new_order_reference_number() const;
  SharesCount_t          get_nr_shares() const;
  Price_t                get_price() const;
};

class OrderCancelMessage : public Message
{
public:
  OrderReferenceNumber_t get_order_reference_number() const;
  SharesCount_t          get_nr_shares() const;
};

class OrderDeleteMessage : public Message
{
public:
  OrderReferenceNumber_t get_order_reference_number() const;
};

class TradeMessage : public Message
{
public:
  OrderReferenceNumber_t get_order_reference_number() const;
  OrderType              get_order_type() const;
  SharesCount_t          get_nr_shares() const;
  Stock_t                get_stock() const;
  Price_t                get_price() const;
  MatchNumber_t          get_match_number() const;
};

class BrokenTradeMessage : public Message
{
public:
  MatchNumber_t get_match_number() const;
};

std::ostream &operator<<(std::ostream &ss, const Message &message);

constexpr auto MESSAGE_LENGTH_SIZE = 2;
constexpr auto MAX_PAYLOAD_SIZE    = 512; // TODO way too big but not a big deal

class MessageReader
{
public:
  MessageReader(std::string filename);

  bool is_done() const;
  bool next(Message &message);

private:
  bool read(std::size_t length);

  // std::ifstream m_ifs;
  //  unsigned char                 m_buffer[MAX_PAYLOAD_SIZE]{};
  boost::iostreams::mapped_file m_mapped_file;
  const unsigned char          *m_file_start{};
  const unsigned char          *m_file_end{};
};

struct OrderExecution
{
  SharesCount_t m_nr_shares{};
  Price_t       m_price{};
};

struct Order
{
  OrderReferenceNumber_t m_reference_number{};
  OrderType              m_type{};
  SharesCount_t          m_nr_shares{};
  Stock_t                m_stock{};
  Price_t                m_price{};
};

struct Execution
{
  OrderReferenceNumber_t m_reference_number{};
  OrderType              m_type{};
  SharesCount_t          m_nr_shares{};
  MatchNumber_t          m_match_num{};
  Stock_t                m_stock{};
  Price_t                m_price{};
};

class MessageHandler
{
public:
  MessageHandler();
  void handle_message(const Message &message);

private:
  void report();

  using OrderMap     = btree::map<OrderReferenceNumber_t, Order>;
  using ExecutionMap = btree::map<MatchNumber_t, Execution>;

  OrderMap     m_orders;
  ExecutionMap m_executions;
  //  std::map<OrderReferenceNumber_t, Order>                 m_orders;
  // std::map<MatchNumber_t, Execution>                      m_executions;
  //  std::unordered_map<OrderReferenceNumber_t, Order> m_orders;
  //  std::unordered_map<MatchNumber_t, Execution>      m_executions;

  Timestamp_t m_last_report_timestamp{};
};

} // namespace ITCH
