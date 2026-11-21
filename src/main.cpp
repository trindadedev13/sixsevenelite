
#include <chrono>
#include <cstdlib>
#include <format>
#include <print>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include "sse/api.hpp"
#include "sse/run_code.hpp"

int main(void)
{
    auto const bot_token = std::getenv("SE_BOT_TOKEN");
    if (!bot_token)
        throw std::runtime_error("no bot token provided");

    sse::telegram::api api{bot_token};
    std::int64_t offset{0};

    std::unordered_map<
        std::int64_t,
        std::unordered_map<std::int64_t, nlohmann::json>
    > messages;

    std::println("api running...");

    while (true)
    {
        try
        {
            auto const updates = api.get_updates(offset);

            for (auto const &raw_update : updates)
            {
                sse::telegram::update update{raw_update};

                offset = update.id() + 1;

                if (!update.has_message())
                    continue;

                auto const message = update.get_message();

                messages[message.chat_id()][message.id()] = message.json();

                if (!message.has_text())
                    continue;

                auto const &text = message.text();

                if (text.starts_with("/reply "))
                {
                    auto const reply_to = message.reply_to_message_id();

                    if (!reply_to)
                    {
                        api.reply_message(
                            message.chat_id(),
                            "Responda a uma mensagem para usar esse comando.",
                            message.id()
                        );

                        continue;
                    }

                    auto const reply_text = text.substr(7);

                    api.reply_message(
                        message.chat_id(),
                        reply_text,
                        *reply_to
                    );

                    api.delete_message(
                        message.chat_id(),
                        message.id()
                    );
                }
                else if (text.starts_with("/py "))
                {
                    auto const code = text.substr(4);
                    auto const out = sse::run_py(code);

                    std::println("python:\n{}\noutput:\n{}", code, out);

                    api.reply_message(message.chat_id(), out, message.id());
                }
                else if (text.starts_with("/js "))
                {
                    auto const code = text.substr(4);
                    auto const out = sse::run_js(code);

                    std::println("javascript:\n{}\noutput:\n{}", code, out);

                    api.reply_message(message.chat_id(), out, message.id());
                }
                else if (text.starts_with("/kt "))
                {
                    auto const code = text.substr(4);
                    auto const out = sse::run_kt(code);

                    std::println("kotlin:\n{}\noutput:\n{}", code, out);

                    api.reply_message(message.chat_id(), out, message.id());
                }
                else if (text.starts_with("/c "))
                {
                    auto const code = text.substr(3);
                    auto const out = sse::run_c(code);

                    std::println("c:\n{}\noutput:\n{}", code, out);

                    api.reply_message(message.chat_id(), out, message.id());
                }
                else if (text.starts_with("/jv "))
                {
                    auto const code = text.substr(4);
                    auto const out = sse::run_java(code);

                    std::println("java:\n{}\noutput:\n{}", code, out);

                    api.reply_message(message.chat_id(), out, message.id());
                }
                else if (text == "/id")
                {
                    auto const reply_to = message.reply_to_message_id();

                    if (reply_to)
                    {
                        auto const chat = messages.find(message.chat_id());

                        if (chat == messages.end() ||
                            !chat->second.contains(*reply_to))
                        {
                            api.reply_message(
                                message.chat_id(),
                                "Não encontrei essa mensagem no histórico do bot.",
                                message.id()
                            );

                            continue;
                        }

                        auto const &target = chat->second.at(*reply_to);

                        if (!target.contains("from") ||
                            !target.at("from").contains("id"))
                        {
                            api.reply_message(
                                message.chat_id(),
                                "Não foi possível identificar o autor dessa mensagem.",
                                message.id()
                            );

                            continue;
                        }

                        auto const &from = target.at("from");

                        auto const user_id = from.at("id").get<std::int64_t>();

                        auto const username = from.contains("username")
                            ? from.at("username").get<std::string>()
                            : "none";

                        api.reply_message(
                            message.chat_id(),
                            std::format("@{} id: {}", username, user_id),
                            message.id()
                        );
                    }
                    else
                    {
                        api.reply_message(
                            message.chat_id(),
                            std::format("chat id: {}", message.chat_id()),
                            message.id()
                        );
                    }
                }
            }
        }
        catch (std::exception const &e)
        {
            std::println("Exception: {}", e.what());
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}
