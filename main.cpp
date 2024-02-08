#include "Message.h"
#include <iostream>

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage:" << std::endl
              << "\tITCH50_Hourly_VWAP <unzipped NASDAQ ITCH 5.0 file>" << std::endl
              << "\tExample: ITCH50_Hourly_VWAP 01302019.NASDAQ_ITCH50" << std::endl;
  }

  std::ios::sync_with_stdio(false);

  try
  {
    const auto filename{argv[1]};
    auto       message         = ITCH::Message{};
    auto       message_reader  = std::make_shared<ITCH::MessageReader>(filename);
    auto       message_handler = ITCH::MessageHandler(message_reader);

    while (message_reader->next(message))
    {
      message_handler.handle_message(message);
    }
  }
  catch (const std::exception &ex)
  {
    std::cerr << "An exception occurred: " << ex.what() << std::endl;
    return -1;
  }
  catch (...)
  {
    std::cout << "An unknown exception occurred!" << std::endl;
    return -1;
  }

  return 0;
}
