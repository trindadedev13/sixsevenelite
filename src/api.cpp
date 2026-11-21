
#include "sse/api.hpp"

#include <cstddef>
#include <curl/curl.h>
#include <curl/easy.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace sse::telegram
{

message::message(nlohmann::json const &data)
    : data{data}
{
}

std::int64_t message::id() const
{
    return data.at("message_id").get<std::int64_t>();
}

std::int64_t message::chat_id() const
{
    return data.at("chat").at("id").get<std::int64_t>();
}

std::string const &message::text() const
{
    return data.at("text").get_ref<std::string const &>();
}

std::optional<std::int64_t> message::reply_to_message_id() const
{
    if (!data.contains("reply_to_message"))
        return std::nullopt;

    return data.at("reply_to_message")
        .at("message_id")
        .get<std::int64_t>();
}

std::int64_t message::user_id() const
{
    return data.at("from").at("id").get<std::int64_t>();
}


std::string const &message::user_first_name() const
{
    return data.at("from").at("first_name").get_ref<std::string const &>();
}

std::string const &message::user_last_name() const
{
    return data.at("from").at("last_name").get_ref<std::string const &>();
}

std::string const &message::username() const
{
    return data.at("from").at("username").get_ref<std::string const &>();
}

bool message::has_text() const
{
    return data.contains("text") && data.at("text").is_string();
}

bool message::has_user() const
{
    return data.contains("from") && data.at("from").contains("id");
}

nlohmann::json const &message::json() const
{
    return data;
}

// update

update::update(nlohmann::json const &data)
    : data{data}
{
}

std::int64_t update::id() const
{
    return data.at("update_id").get<std::int64_t>();
}

bool update::has_message() const
{
    return data.contains("message") && data.at("message").is_object();
}

message update::get_message() const
{
    return message{data.at("message")};
}

nlohmann::json const &update::json() const
{
    return data;
}

// api

api::api(std::string const &token_)
    : token{std::move(token_)},
      curl{curl_easy_init()}
{
    if (!curl)
        throw std::runtime_error("failed to init CURL.");
}

api::~api()
{
    if (curl)
        curl_easy_cleanup(curl);
}

std::size_t api::write_callback(
    char *data,
    std::size_t size,
    std::size_t nmemb,
    void *udata
)
{
    auto *resp = static_cast<std::string *>(udata);
    auto const bytes = size * nmemb;
    resp->append(data, bytes);
    return bytes;
}

nlohmann::json api::request(
    std::string_view method,
    nlohmann::json const &parameters
)
{
    auto const url =
        "https://api.telegram.org/bot" + token + "/" + std::string(method);

    std::string body;
    std::string response;

    for (auto const &[key, value] : parameters.items())
    {
        if (!body.empty())
            body += '&';

        auto const key_encoded = curl_easy_escape(
            curl,
            key.c_str(),
            static_cast<int>(key.size())
        );

        auto const value_string = value.is_string()
            ? value.get<std::string>()
            : value.dump();

        auto const value_encoded = curl_easy_escape(
            curl,
            value_string.c_str(),
            static_cast<int>(value_string.size())
        );

        if (!key_encoded || !value_encoded)
        {
            if (key_encoded)
                curl_free(key_encoded);

            if (value_encoded)
                curl_free(value_encoded);

            throw std::runtime_error("Failed to encode parameters");
        }

        body += key_encoded;
        body += '=';
        body += value_encoded;

        curl_free(key_encoded);
        curl_free(value_encoded);
    }

    curl_easy_reset(curl);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 70L);

    auto const result = curl_easy_perform(curl);

    if (result != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(result));

    long status_code = 0;

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

    if (status_code != 200)
    {
        throw std::runtime_error(
            "Telegram HTTP error (" +
            std::to_string(status_code) + "): " + response
        );
    }

    auto const json = nlohmann::json::parse(response);

    if (!json.value("ok", false))
    {
        throw std::runtime_error(
            json.value("description", "Telegram API error")
        );
    }

    return json.at("result");
}

nlohmann::json api::get_updates(std::int64_t offset, int timeout)
{
    return request(
        "getUpdates",
        {
            {"offset", offset},
            {"timeout", timeout},
            {"allowed_updates", {"message"}}
        }
    );
}

nlohmann::json api::send_message(std::int64_t chat_id, std::string const &text)
{
    return request("sendMessage", {
        {"chat_id", chat_id},
        {"text", text}
    });
}

nlohmann::json api::reply_message(
    std::int64_t chat_id,
    std::string const &text,
    std::int64_t reply_to
)
{
    return request(
        "sendMessage",
        {
            {"chat_id", chat_id},
            {"text", text},
            {"reply_parameters", {{"message_id", reply_to}}}
        }
    );
}

nlohmann::json api::send_photo(
    std::int64_t chat_id,
    std::string const &photo,
    std::string const &caption
)
{
    return request("sendPhoto", {
        {"chat_id", chat_id},
        {"photo", photo},
        {"caption", caption}
    });
}

nlohmann::json api::send_document(
    std::int64_t chat_id,
    std::string const &document,
    std::string const &caption
)
{
    return request("sendDocument", {
        {"chat_id", chat_id},
        {"document", document},
        {"caption", caption}
    });
}

nlohmann::json api::delete_message(
    std::int64_t chat_id,
    std::int64_t message_id
)
{
    return request(
        "deleteMessage",
        {
            {"chat_id", chat_id},
            {"message_id", message_id}
        }
    );
}

} // namespace sse::telegram
