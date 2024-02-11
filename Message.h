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

#include "ankerl/unordered_dense.h"
#include <boost/container/flat_map.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <span>
#include <string_view>

namespace ITCH
{

// TODO Benchmark further -> compare with sparse map and flat map
template <typename K, typename V> using HashMap = ankerl::unordered_dense::segmented_map<K, V>;
// template <typename K, typename V> using HashMap = ankerl::unordered_dense::map<K, V>;
// template <typename K, typename V> using HashMap = boost::unordered_map<K, V>;
// TODO Segfaulting at key comparison during erase!? Fixable?
// template <typename K, typename V> using HashMap = btree::map<K, V>;
// template <typename K, typename V> using TreeMap = boost::container::map<K, V>;
template <typename K, typename V> using TreeMap = boost::container::flat_map<K, V>;

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
  RetailInterest                              = 'N',
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

using Attribution_t          = std::string_view;
using MatchNumber_t          = std::uint64_t;
using OrderReferenceNumber_t = std::uint64_t;
using Price_t                = double;
using SharesCount_t          = std::uint32_t;
using StockLocate_t          = std::uint16_t;
// using Stock_t                = std::string;   // TODO 9% cycle time, change to char[]?
using Stock_t          = std::string_view;
using Timestamp_t      = std::uint64_t; // 48-bit
using TrackingNumber_t = std::uint16_t;

class Message
{
public:
  Message() = default;

  Message(std::span<const unsigned char> raw_data, size_t pos) : m_raw_data(raw_data), m_pos(pos)
  {
  }

  //  virtual ~Message()                  = default;
  //  Message(const Message &)            = default;
  //  Message &operator=(const Message &) = default;
  //  Message(Message &&)                 = default;
  //  Message &operator=(Message &&)      = default;

  std::size_t      get_offset() const;
  std::size_t      get_length() const;
  MessageType      get_type() const;
  StockLocate_t    get_stock_locate() const;
  TrackingNumber_t get_tracking_number() const;
  Timestamp_t      get_timestamp() const;

protected:
  std::span<const unsigned char> m_raw_data;

private:
  size_t m_pos{};
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

class MessageReader
{
public:
  MessageReader(std::string filename);

  bool next(Message &message);
  bool read(Message &message, size_t pos) const;

private:
  // std::ifstream m_ifs;
  boost::iostreams::mapped_file m_file;
  std::size_t                   m_pos{};
  const unsigned char          *m_data;
  const std::size_t             m_size{};
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

struct VolumePrice
{
  double volume{};
  double price{};
};

class MessageHandler
{
public:
  MessageHandler(std::shared_ptr<MessageReader> message_reader);
  ~MessageHandler();
  void handle_message(const Message &message);

private:
  bool construct_order(OrderReferenceNumber_t ref_num, Order &order) const;
  void execute_order(Stock_t stock, SharesCount_t nr_shares, Price_t price);
  void report(const Timestamp_t &current_time);

  using OrderMap            = HashMap<OrderReferenceNumber_t, size_t>;
  using StockVolumePriceMap = TreeMap<Stock_t, VolumePrice>;

  std::shared_ptr<MessageReader> m_message_reader;
  OrderMap                       m_orders;
  StockVolumePriceMap            m_stocks;
  Timestamp_t                    m_last_report_time{};
};

} // namespace ITCH
