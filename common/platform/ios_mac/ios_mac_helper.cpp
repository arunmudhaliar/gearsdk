#if defined(__APPLE__)
#include "ios_mac_helper.h"

extern "C" {
namespace gsdk {
namespace platform {
namespace apple {
// Function to convert CFString to C string
char* ios_mac_helper::cf_string_to_c_string(CFStringRef cf_string) {
	if (!cf_string)
		return NULL;

	const char* c_str = CFStringGetCStringPtr(cf_string, kCFStringEncodingUTF8);
	if (!c_str) {
		// If the string is not available as a C string, convert it manually
		CFIndex length = CFStringGetLength(cf_string) * 4;	// UTF-8 can be up to 4 bytes per character
		char* buffer = (char*) malloc(length + 1);			// Allocate memory
		if (buffer && CFStringGetCString(cf_string, buffer, length + 1, kCFStringEncodingUTF8)) {
			return buffer;	// Caller must free
		}
		free(buffer);  // Free on failure
		return NULL;
	}
	return strdup(c_str);  // Caller must free
}

// Function to copy a C string
char* ios_mac_helper::c_string_copy(const char* c_str) {
	if (!c_str)
		return NULL;
	size_t length = strlen(c_str);
	char* copy = (char*) malloc(length + 1);  // Allocate memory
	if (copy) {
		strcpy(copy, c_str);
	}
	return copy;  // Caller must free
}

// Function to get the locale
char* ios_mac_helper::get_locale() {
	// Step 1: Get the array of preferred languages
	CFArrayRef preferred_languages = CFLocaleCopyPreferredLanguages();
	if (!preferred_languages || CFArrayGetCount(preferred_languages) == 0) {
		printf("Failed to retrieve preferred languages.\n");
		if (preferred_languages)
			CFRelease(preferred_languages);
		return NULL;
	}

	// Step 2: Get the first preferred language
	CFStringRef preferred_language = (CFStringRef) CFArrayGetValueAtIndex(preferred_languages, 0);
	if (!preferred_language) {
		CFRelease(preferred_languages);
		return NULL;
	}

	// Step 3: Convert CFString to C string
	char* return_locale = cf_string_to_c_string(preferred_language);  // Caller must free

	// Clean up
	CFRelease(preferred_languages);

	return return_locale;
}

char* ios_mac_helper::get_duid(const char* service_name, const char* account) {
	CFTypeRef result = NULL;
	char* identifier_string = NULL;
	// Create a query to fetch the password
	CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	if (!query) {
		printf("Failed to create query dictionary.\n");
		return NULL;
	}
	CFStringRef service_key = CFStringCreateWithCString(NULL, service_name, kCFStringEncodingUTF8);
	CFStringRef account_key = CFStringCreateWithCString(NULL, account, kCFStringEncodingUTF8);

	if (!service_key || !account_key) {
		printf("Failed to create CFStrings for keys.\n");
		CFRelease(query);
		if (service_key)
			CFRelease(service_key);
		if (account_key)
			CFRelease(account_key);
		return NULL;
	}

	CFDictionaryAddValue(query, kSecClass, kSecClassGenericPassword);
	CFDictionaryAddValue(query, kSecAttrService, service_key);
	CFDictionaryAddValue(query, kSecAttrAccount, account_key);
	CFDictionaryAddValue(query, kSecReturnData, kCFBooleanTrue);
	CFDictionaryAddValue(query, kSecMatchLimit, kSecMatchLimitOne);

	// Attempt to fetch the item
	OSStatus status = SecItemCopyMatching(query, (CFTypeRef*) &result);
	CFRelease(query);

	if (status == errSecSuccess) {
		if (result) {
			CFDataRef data = (CFDataRef) result;
			CFStringRef cf_string = CFStringCreateWithBytes(NULL, CFDataGetBytePtr(data), CFDataGetLength(data), kCFStringEncodingUTF8, false);
			identifier_string = cf_string_to_c_string(cf_string);
			CFRelease(cf_string);
		}
		CFRelease(result);
	} else {
		printf("SecItemCopyMatching returned: %d\n", status);

		// Generate a new UUID
		CFUUIDRef uuid = CFUUIDCreate(NULL);
		CFStringRef uuid_string = CFUUIDCreateString(NULL, uuid);
		identifier_string = cf_string_to_c_string(uuid_string);

		if (identifier_string) {
			// Create and store the new identifier in the Keychain
			CFMutableDictionaryRef new_query = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			CFDictionaryAddValue(new_query, kSecClass, kSecClassGenericPassword);
			CFDictionaryAddValue(new_query, kSecAttrService, service_key);
			CFDictionaryAddValue(new_query, kSecAttrAccount, account_key);
			CFDataRef password_data = CFDataCreate(NULL, (const UInt8*) identifier_string, strlen(identifier_string));

			if (password_data) {
				CFDictionaryAddValue(new_query, kSecValueData, password_data);
				OSStatus add_status = SecItemAdd(new_query, NULL);
				if (add_status != errSecSuccess) {
					printf("Error adding item to Keychain: %d\n", add_status);
				}
				CFRelease(password_data);
			}

			CFRelease(new_query);
		}

		CFRelease(uuid);
		CFRelease(uuid_string);
	}

	// Clean up
	CFRelease(service_key);
	CFRelease(account_key);

	return identifier_string;  // Caller must free this
}
}  // namespace apple
}  // namespace platform
}  // namespace gsdk
}
#endif
/*
int main() {
	char *duid = get_duid();
	if (duid) {
		printf("DUID: %s\n", duid);
		free(duid); // Free the allocated memory for the DUID
	} else {
		printf("Failed to retrieve or create DUID.\n");
	}

	char *locale = get_locale();
	if (locale) {
		printf("Current Locale: %s\n", locale);
		free(locale); // Don't forget to free the allocated memory
	} else {
		printf("Failed to retrieve locale.\n");
	}
	return 0;
}
*/
