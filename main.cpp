#include "Message.h"
#include <iostream>

int main(int argc, char *argv[])
{
  std::ios::sync_with_stdio(false);

  try
  {
    const auto filename{argv[1]};

    ITCH::MessageReader  mr(filename);
    ITCH::MessageHandler mh;
    ITCH::Message        message;

    while (mr.next(message))
    {
      mh.handle_message(message);
    }
  }
  catch (std::exception &ex)
  {
    std::cerr << "An exception occurred: " << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cout << "An unknown exception occurred!" << std::endl;
  }

  return 0;
}