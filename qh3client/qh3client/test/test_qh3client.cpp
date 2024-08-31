#include "../networkcommon/source/message.hpp"
#include "../qh3client.hpp"
#include "../qh3client_helper.hpp"

#include <functional>
#include <future>
#include <gtest/gtest.h>
#include <rapidjson/error/en.h>
#include <rapidjson/rapidjson.h>

using namespace client;

namespace ft_test_qh3client {
struct mock_app_state {
	qstring host;
	int port;
	qstring arg;
	type_qh3client_helper_cb async_cb;
	int retry;

	mock_app_state() : host("15.206.79.30"), port(4004), arg("test_arg"), async_cb(nullptr), retry(3) {}

	mock_app_state(const qstring& h, int p, const qstring& a, const qstring& d, type_qh3client_helper_cb cb, int r) : host(h), port(p), arg(a), async_cb(cb), retry(r) {}

	~mock_app_state() {}
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
		app_state->async_cb = nullptr;	// or set to a mock callback
		app_state->retry = 3;			// example retry count
	}

	void TearDown() override { GX_DELETE(app_state); }
};

// Test case for qh3client::send_request
TEST_P(functional_test_qh3client, test_send_request) {
	int iteration = GetParam();
	// DEBUG_RAW(LOG_LEVEL_0, "Running test iteration: %d", iteration + 1);

	std::vector<qstring> json_objects = {R"({"pid":"","token":"","details":{"sys_name":"Linux","node_name":"server01.local","release":"5.15.0-56-generic","arch":"x86_64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Windows","node_name":"DESKTOP-1234ABCD","release":"10.0.19044","arch":"amd64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Darwin","node_name":"Johns-MBP.local","release":"22.3.0","arch":"arm64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Ubuntu","node_name":"ubuntu-vm.local","release":"20.04","arch":"x86_64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"FreeBSD","node_name":"freebsd-server.local","release":"13.1-RELEASE","arch":"i386"}})"};

	qstring json_str_object = json_objects[iteration % json_objects.size()];
	Document doc;
	ParseResult ok = doc.Parse(json_str_object.c_str());
	EXPECT_TRUE(ok) << "JSON Parse FAILED !!!";
	if (!ok) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "JSON parse error: %s (%u)\n", rapidjson::GetParseError_En(ok.Code()), ok.Offset());
	}
	rq_msg_user_get user_get_msg_rq;
	EXPECT_TRUE(user_get_msg_rq.deserialize(doc)) << "deserialize FAILED !!!";

	qstring json_str;
	user_get_msg_rq.get_json_string(json_str);

	qh3client* new_client = DEBUG_NEW qh3client(app_state->host, app_state->port, reinterpret_cast<void*>(&app_state->arg));

	// Call the method under test
	auto req = conn_io_req_res::create("/user_get", json_str);
	int result = new_client->send_request(req, app_state->async_cb);
	GX_DELETE(req);
	EXPECT_EQ(0, result) << "send_request FAILED !!!";	// Ensure the request was sent successfully

	// Capture the response_received state
	bool response_received = new_client->conn_io->res_received;

	// Check if the response_received is as expected (true/false depending on your logic)
	EXPECT_TRUE(response_received) << "response_received is false !!!";	 // or EXPECT_FALSE if it should be false

	const conn_io_req_res::payload& payload = new_client->conn_io->response->get_payload();
	// DEBUG_RAW(LOG_LEVEL_0, "%.*s", payload.buffer.length(), payload.buffer.c_str());

	// Verify that DEBUG_PRINT was called with expected arguments
	// (Use a logging mock or check log output if possible)

	// Verify cleanup was called (depends on the implementation of on_post_send_cleanup)
	EXPECT_NO_THROW(new_client->on_post_send_cleanup()) << "on_post_send_cleanup call failure !!!";

	// Verify that the client was deleted
	GX_DELETE(new_client);	// Ensure proper deletion if necessary

	// If using memory management libraries like GX_DELETE, include verification if needed
}

// Test case for qh3client_helper::send_request
TEST_P(functional_test_qh3client, test_qh3client_helper_send_request_with_callback) {
	int iteration = GetParam();
	// DEBUG_RAW(LOG_LEVEL_0, "Running test iteration: %d", iteration + 1);

	std::vector<qstring> json_objects = {R"({"pid":"","token":"","details":{"sys_name":"Linux","node_name":"server01.local","release":"5.15.0-56-generic","arch":"x86_64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Windows","node_name":"DESKTOP-1234ABCD","release":"10.0.19044","arch":"amd64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Darwin","node_name":"Johns-MBP.local","release":"22.3.0","arch":"arm64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Ubuntu","node_name":"ubuntu-vm.local","release":"20.04","arch":"x86_64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"FreeBSD","node_name":"freebsd-server.local","release":"13.1-RELEASE","arch":"i386"}})"};

	qstring json_str_object = json_objects[iteration % json_objects.size()];
	Document doc;
	ParseResult ok = doc.Parse(json_str_object.c_str());
	EXPECT_TRUE(ok) << "JSON Parse FAILED !!!";
	if (!ok) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "JSON parse error: %s (%u)\n", rapidjson::GetParseError_En(ok.Code()), ok.Offset());
	}
	rq_msg_user_get user_get_msg_rq;
	EXPECT_TRUE(user_get_msg_rq.deserialize(doc)) << "deserialize FAILED !!!";

	qstring json_str;
	user_get_msg_rq.get_json_string(json_str);

	// DEBUG_RAW(LOG_LEVEL_0, "JSON string: %s", json_str.c_str());

	std::promise<void> callback_invoked;
	std::future<void> callback_future = callback_invoked.get_future();
	bool success_flag = false;
	qstring token_value;

	//     // Act
	int result = qh3client_helper::send_request<client::qh3client>(
		app_state->host, app_state->port, conn_io_req_res::create("/user_get", json_str),
		[&callback_invoked, &success_flag, &token_value](conn_io_req_res* request, conn_io_req_res* response, void*, void*, bool success) {
			success_flag = success;
			bool validate = response->validate();
			EXPECT_TRUE(validate) << "crc validation FAILED !!!";

			// DEBUG_RAW(LOG_LEVEL_0, "Callback called: %s", response->get_payload().buffer.c_str());

			conn_io_req_res::header* token_header = response->get_header("token");
			if (token_header) {
				token_value = token_header->value;
			}

			// Notify the test that the callback was invoked
			callback_invoked.set_value();  // This won't cause memory leaks. Since the main thread will wait for the worker thread to finish its job.
		},
		1);

	EXPECT_EQ(0, result);  // Ensure the request was sent successfully

	// Wait for the callback to be invoked
	callback_future.wait();

	// Assert
	EXPECT_TRUE(success_flag);	// Ensure the request succeeded
	// Perform the assertion after the async operation completes
	// DEBUG_RAW(LOG_LEVEL_0, "Token value: %s", token_value.c_str());
	EXPECT_TRUE(token_value.length() > 0) << "Token value should not be empty";
}

// Test case for qh3client_helper::send_async_request
TEST_P(functional_test_qh3client, test_qh3client_helper_send_async_request_with_callback) {
	int iteration = GetParam();
	// DEBUG_RAW(LOG_LEVEL_0, "Running test iteration: %d", iteration + 1);

	std::vector<qstring> json_objects = {R"({"pid":"","token":"","details":{"sys_name":"Linux","node_name":"server01.local","release":"5.15.0-56-generic","arch":"x86_64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Windows","node_name":"DESKTOP-1234ABCD","release":"10.0.19044","arch":"amd64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Darwin","node_name":"Johns-MBP.local","release":"22.3.0","arch":"arm64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"Ubuntu","node_name":"ubuntu-vm.local","release":"20.04","arch":"x86_64"}})",
										 R"({"pid":"","token":"","details":{"sys_name":"FreeBSD","node_name":"freebsd-server.local","release":"13.1-RELEASE","arch":"i386"}})"};

	qstring json_str_object = json_objects[iteration % json_objects.size()];
	Document doc;
	ParseResult ok = doc.Parse(json_str_object.c_str());
	EXPECT_TRUE(ok) << "JSON Parse FAILED !!!";
	if (!ok) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "JSON parse error: %s (%u)\n", rapidjson::GetParseError_En(ok.Code()), ok.Offset());
	}
	rq_msg_user_get user_get_msg_rq;
	EXPECT_TRUE(user_get_msg_rq.deserialize(doc)) << "deserialize FAILED !!!";

	qstring json_str;
	user_get_msg_rq.get_json_string(json_str);

	// DEBUG_RAW(LOG_LEVEL_0, "JSON string: %s", json_str.c_str());

	std::promise<void> callback_invoked;
	std::future<void> callback_future = callback_invoked.get_future();
	bool success_flag = false;
	qstring token_value;

	// Act
	int result = qh3client_helper::send_async_request<client::qh3client>(
		app_state->host, app_state->port, conn_io_req_res::create("/user_get", json_str), nullptr,
		[&callback_invoked, &success_flag, &token_value](conn_io_req_res* request, conn_io_req_res* response, void*, void*, bool success) {
			success_flag = success;
			bool validate = response->validate();
			EXPECT_TRUE(validate) << "crc validation FAILED !!!";
			// DEBUG_RAW(LOG_LEVEL_0, "Callback called: %s", response->get_payload().buffer.c_str());
			conn_io_req_res::header* token_header = response->get_header("token");
			if (token_header) {
				token_value = token_header->value;
			}
			// Notify the test that the callback was invoked
			// callback_invoked.set_value();    // Will lead to memory leak since the main thread will exit immediately after this.
		},
		1,
		[&callback_invoked](void* arg) {
			// Cleanup logic, if necessary
			// DEBUG_RAW(LOG_LEVEL_0, "Cleaning up thread.");
			callback_invoked.set_value();  // Notify the test that the callback was invoked.
										   // This needs to be done here else ther will be memory leaks on exit since the main will exit immediately.
		});

	EXPECT_EQ(0, result) << "send_async_request returned non zero !!!";	 // Ensure the request was sent successfully

	// Wait for the callback to be invoked
	callback_future.wait();

	// Assert
	EXPECT_TRUE(success_flag) << "success_flag is FALSE !!!";  // Ensure the request succeeded
	// Perform the assertion after the async operation completes
	// DEBUG_RAW(LOG_LEVEL_0, "Token value: %s", token_value.c_str());
	EXPECT_TRUE(token_value.length() > 0) << "Token value should not be empty";
}

// Instantiate the test suite with the number of iterations
INSTANTIATE_TEST_SUITE_P(runs_five_time, functional_test_qh3client, ::testing::Range(0, 5));
};	// namespace ft_test_qh3client