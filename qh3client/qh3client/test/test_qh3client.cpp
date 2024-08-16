#include <functional>
#include <future>
#include <gtest/gtest.h>
#include "../qh3client.hpp"
#include "../qh3client_helper.hpp"
#include "../networkcommon/source/message.hpp"

using namespace client;

namespace ft_test_qh3client {
// Mock app state definition
struct mock_app_state {
    qstring host;
    int port;
    qstring arg;
    conn_io_req_res* data = nullptr;
    type_qh3client_helper_cb async_cb;
    int retry;

    // Constructor to initialize with defaults if needed
    mock_app_state() 
        : host("15.206.79.30"), port(4004), arg("test_arg"), 
          async_cb(nullptr), retry(3) {}

    // Optionally, you can provide a parameterized constructor for custom values
    mock_app_state(const qstring& h, int p, const qstring& a, 
               const qstring& d, type_qh3client_helper_cb cb, int r)
        : host(h), port(p), arg(a), async_cb(cb), retry(r) {}

    ~mock_app_state() {
        GX_DELETE(data);
    }
};

class functional_test_qh3client : public ::testing::TestWithParam<int> {
protected:
    // Mock or real dependencies
    mock_app_state* app_state;

    void SetUp() override {
        // Initialize app state
        app_state = DEBUG_NEW mock_app_state();
        app_state->host = "15.206.79.30";
        app_state->port = 4004;
        app_state->arg = "test_arg";

        rq_msg_user_get user_get_msg_rq;
        user_get_msg_rq.sys_name = essentials::get_sysname();
        user_get_msg_rq.node_name = essentials::get_device_name();
        user_get_msg_rq.arch = essentials::get_device_arch();
        user_get_msg_rq.release = essentials::get_device_release_str();

        qstring json_str;
        user_get_msg_rq.get_json_string(json_str);
    
        app_state->data = conn_io_req_res::create("/whoami", json_str);
        app_state->async_cb = nullptr; // or set to a mock callback
        app_state->retry = 3; // example retry count
    }

    void TearDown() override {
        GX_DELETE(app_state);
    }
};

TEST_P(functional_test_qh3client, test_send_request) {
    int iteration = GetParam();
    DEBUG_RAW(LOG_LEVEL_0, "Running test iteration: %d", iteration + 1);

    // Mock qh3client
    qh3client* new_client = DEBUG_NEW qh3client(app_state->host, app_state->port, reinterpret_cast<void*>(&app_state->arg));
    
    // Set expectations for the mock object or verify results
    // E.g., using gmock to set expectations on send_request
    
    // Call the method under test
    new_client->send_request(app_state->data, app_state->async_cb);
    
    // Capture the response_received state
    bool response_received = new_client->conn_io->res_received;
    
    // Check if the response_received is as expected (true/false depending on your logic)
    EXPECT_TRUE(response_received); // or EXPECT_FALSE if it should be false
    
    const conn_io_req_res::payload& payload = new_client->conn_io->response->get_payload();
    DEBUG_RAW(LOG_LEVEL_0, "%.*s", payload.buffer.length(), payload.buffer.c_str());

    // Verify that DEBUG_PRINT was called with expected arguments
    // (Use a logging mock or check log output if possible)
    
    // Verify cleanup was called (depends on the implementation of on_post_send_cleanup)
    EXPECT_NO_THROW(new_client->on_post_send_cleanup());
    
    // Verify that the client was deleted
    GX_DELETE(new_client); // Ensure proper deletion if necessary
    
    // If using memory management libraries like GX_DELETE, include verification if needed
}

// Test case for send_async_request
TEST_P(functional_test_qh3client, test_send_async_request_with_callback) {
    int iteration = GetParam();
    DEBUG_RAW(LOG_LEVEL_0, "Running test iteration: %d", iteration + 1);

    rq_msg_user_get user_get_msg_rq;
    user_get_msg_rq.sys_name = essentials::get_sysname();
    user_get_msg_rq.node_name = essentials::get_device_name();
    user_get_msg_rq.arch = essentials::get_device_arch();
    user_get_msg_rq.release = essentials::get_device_release_str();

    qstring json_str;
    user_get_msg_rq.get_json_string(json_str);

    std::promise<void> callback_invoked;
    std::future<void> callback_future = callback_invoked.get_future();
    bool success_flag = false;
    qstring token_value;

    // Act
    qh3client_helper::send_async_request<client::qh3client>(
        app_state->host, app_state->port, conn_io_req_res::create("/user_get", json_str), nullptr,
        [&callback_invoked, &success_flag, &token_value](conn_io_req_res* response, void*, void*, bool success) {
            success_flag = success;
            bool validate = response->validate();
            EXPECT_TRUE(validate) << "crc validation FAILED !!!";

            DEBUG_RAW(LOG_LEVEL_0, "Callback called: %s", response->get_payload().buffer.c_str());

            conn_io_req_res::header* token_header = response->get_header("token");
            if (token_header) {
                token_value = token_header->value;
            }

            // Notify the test that the callback was invoked
            callback_invoked.set_value();
        },
        1
    );

    // Wait for the callback to be invoked
    callback_future.wait();

    // Assert
    EXPECT_TRUE(success_flag); // Ensure the request succeeded
    // Perform the assertion after the async operation completes
    DEBUG_RAW(LOG_LEVEL_0, "Token value: %s", token_value.c_str());
    EXPECT_TRUE(token_value.length() > 0) << "Token value should not be empty";
}

// Instantiate the test suite with the number of iterations
INSTANTIATE_TEST_SUITE_P(runs_five_time, functional_test_qh3client, ::testing::Range(0, 5));
}; // namespace test_qh3client