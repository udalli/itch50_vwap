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
    auto       message_reader  = ITCH::MessageReader{filename};
    auto       message_handler = ITCH::MessageHandler{};

    while (message_reader.next(message))
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
