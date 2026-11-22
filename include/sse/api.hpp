
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace sse::telegram
{

struct user
{
    explicit user(nlohmann::json const &data);

    std::int64_t id() const;
    std::optional<std::string> username() const;
    std::optional<std::string> first_name() const;
    std::optional<std::string> last_name() const;

private:
    nlohmann::json const &data;
};

struct message
{
    explicit message(nlohmann::json const &data);

    std::int64_t id() const;
    std::int64_t chat_id() const;
    
    std::optional<std::int64_t> reply_to_message_id() const;
    std::optional<std::string> text() const;
    user from() const;

    nlohmann::json const &json() const;

private:
    nlohmann::json const &data;
};

struct update
{
    explicit update(nlohmann::json const &data);

    std::int64_t id() const;

    bool has_message() const;
    message get_message() const;

    nlohmann::json const &json() const;

private:
    nlohmann::json const &data;
};

struct api
{
    explicit api(std::string const &token_);
    ~api();

    nlohmann::json request(std::string_view method, nlohmann::json const &parameters = {});

    nlohmann::json get_updates(std::int64_t offset = 0, int timeout = 60);

    nlohmann::json send_message(std::int64_t chat_id, std::string const &text);

    nlohmann::json
    reply_message(std::int64_t chat_id, std::string const &text, std::int64_t reply_to);

    nlohmann::json
    send_photo(std::int64_t chat_id, std::string const &photo, std::string const &caption = "");

    nlohmann::json send_document(std::int64_t chat_id,
                                 std::string const &document,
                                 std::string const &caption = "");

    nlohmann::json delete_message(std::int64_t chat_id, std::int64_t message_id);

private:
    static std::size_t write_callback(char *data, std::size_t size, std::size_t nmemb, void *udata);

    std::string token;
    CURL *curl;
};

} // namespace sse::telegram
