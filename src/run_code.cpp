#include "sse/run_code.hpp"

#include <chrono>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace sse
{

std::string run_process(std::string_view executable,
                        std::vector<std::string> const &arguments,
                        std::chrono::seconds timeout)
{
    int pipe_fds[2];

    if (pipe(pipe_fds) == -1)
        throw std::runtime_error("can't create pipe");

    pid_t pid = fork();

    if (pid == -1)
    {
        close(pipe_fds[0]);
        close(pipe_fds[1]);

        throw std::runtime_error("can't fork");
    }

    if (pid == 0)
    {
        close(pipe_fds[0]);

        dup2(pipe_fds[1], STDOUT_FILENO);
        dup2(pipe_fds[1], STDERR_FILENO);

        close(pipe_fds[1]);

        std::vector<char *> argv;
        argv.reserve(arguments.size() + 2);

        argv.push_back(const_cast<char *>(executable.data()));

        for (auto const &argument : arguments)
            argv.push_back(const_cast<char *>(argument.c_str()));

        argv.push_back(nullptr);

        execvp(argv[0], argv.data());

        _exit(127);
    }

    close(pipe_fds[1]);

    int const flags = fcntl(pipe_fds[0], F_GETFL, 0);
    fcntl(pipe_fds[0], F_SETFL, flags | O_NONBLOCK);

    std::string out;
    std::array<char, 1024> buffer;

    auto const deadline = std::chrono::steady_clock::now() + timeout;

    bool finished = false;
    int status = 0;

    while (!finished)
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            close(pipe_fds[0]);

            return "Execution timed out.";
        }

        struct pollfd pfd{pipe_fds[0], POLLIN | POLLHUP, 0};

        poll(&pfd, 1, 100);

        if (pfd.revents & (POLLIN | POLLHUP))
        {
            while (true)
            {
                auto const bytes = read(pipe_fds[0], buffer.data(), buffer.size());

                if (bytes <= 0)
                    break;

                out.append(buffer.data(), bytes);

                if (out.size() > 3500)
                {
                    kill(pid, SIGKILL);
                    waitpid(pid, nullptr, 0);
                    close(pipe_fds[0]);

                    out.resize(3500);
                    out += "\n[output truncated]";

                    return out;
                }
            }
        }

        auto const result = waitpid(pid, &status, WNOHANG);

        if (result == pid)
            finished = true;
        else if (result == -1 && errno != EINTR)
            finished = true;
    }

    close(pipe_fds[0]);

    if (out.empty())
        return "Execution returned with no output.";

    return out;
}

std::string run_js(std::string_view code)
{
    return run_process("node",
                       {"--permission",
                        "--allow-net",
                        "--allow-fs-read=.",
                        "--allow-fs-write=.",
                        "--allow-child-process",
                        "-e",
                        std::string(code)});
}

std::string run_py(std::string_view code)
{
    return run_process("python", {"-c", std::string(code)});
}

std::string run_kt(std::string_view code)
{
    std::string const path = "./tmp/script.kts";

    {
        std::ofstream file{path};
        if (!file)
            throw std::runtime_error("can't create Kotlin script");

        file << code;
    }

    return run_process("kotlinc", {"-J-Djansi.mode=force", "-script", path});
}

std::string run_c(std::string_view code)
{
    std::string const source = "./tmp/script.c";
    std::string const binary = "./tmp/script_c";

    {
        std::ofstream file{source};
        if (!file)
            throw std::runtime_error("can't create C source");

        file << code;
    }

    auto const compile = run_process("clang", {"-o", binary, source});

    if (compile != "Execution returned with no output.")
        return compile;

    return run_process(binary, {});
}

std::string run_java(std::string_view code)
{
    std::string const directory = "./tmp/java_script";

    if (mkdir(directory.c_str(), 0700) == -1 && errno != EEXIST)
        throw std::runtime_error("can't create Java directory");

    std::string const source = directory + "/Main.java";

    {
        std::ofstream file{source};
        if (!file)
            throw std::runtime_error("can't create Java source");

        file << code;
    }

    auto const compile = run_process("javac", {"-d", directory, source});

    if (compile != "Execution returned with no output.")
        return compile;

    return run_process("java", {"-cp", directory, "Main"});
}

} // namespace sse
