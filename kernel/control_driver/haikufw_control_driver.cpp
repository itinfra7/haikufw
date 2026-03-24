#include "haikufw_abi.h"
#include "haikufw_core.h"

#include <Drivers.h>
#include <KernelExport.h>
#include <module.h>

#include <stdlib.h>
#include <string.h>

int32 api_version = B_CUR_DRIVER_API_VERSION;

static const char* kPublishedDevices[] = {
	HAIKUFW_PUBLISHED_DEVICE_NAME,
	NULL
};

static haikufw_core_module_info* sCoreModule;


static status_t
haikufw_open(const char*, uint32, void** cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
haikufw_close(void*)
{
	return B_OK;
}


static status_t
haikufw_free(void*)
{
	return B_OK;
}


static status_t
haikufw_control(void*, uint32 op, void* data, size_t len)
{
	switch (op) {
		case HAIKUFW_IOCTL_LOAD_RULESET:
		{
			if (data == NULL || len == 0)
				return B_BAD_VALUE;

			void* kernelBuffer = malloc(len);
			if (kernelBuffer == NULL)
				return B_NO_MEMORY;

			status_t status = user_memcpy(kernelBuffer, data, len);
			if (status == B_OK)
				status = sCoreModule->load_ruleset(kernelBuffer, len);

			free(kernelBuffer);
			return status;
		}

		case HAIKUFW_IOCTL_CLEAR_RULESET:
			return sCoreModule->clear_ruleset();

		case HAIKUFW_IOCTL_GET_STATUS:
		{
			if (data == NULL || len < sizeof(haikufw_status))
				return B_BAD_VALUE;

			haikufw_status status {};
			status_t result = sCoreModule->get_status(&status);
			if (result != B_OK)
				return result;

			return user_memcpy(data, &status, sizeof(status));
		}

		case HAIKUFW_IOCTL_GET_VERSION:
		{
			if (data == NULL || len < sizeof(uint32_t))
				return B_BAD_VALUE;

			uint32_t version = HAIKUFW_ABI_VERSION;
			return user_memcpy(data, &version, sizeof(version));
		}

		default:
			return B_DEV_INVALID_IOCTL;
	}
}


static device_hooks sDeviceHooks = {
	haikufw_open,
	haikufw_close,
	haikufw_free,
	haikufw_control,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


status_t
init_hardware(void)
{
	return B_OK;
}


status_t
init_driver(void)
{
	return get_module(HAIKUFW_CORE_MODULE_NAME,
		reinterpret_cast<module_info**>(&sCoreModule));
}


void
uninit_driver(void)
{
	if (sCoreModule != NULL) {
		put_module(HAIKUFW_CORE_MODULE_NAME);
		sCoreModule = NULL;
	}
}


const char**
publish_devices(void)
{
	return kPublishedDevices;
}


device_hooks*
find_device(const char* name)
{
	if (name != NULL && strcmp(name, HAIKUFW_PUBLISHED_DEVICE_NAME) == 0)
		return &sDeviceHooks;
	return NULL;
}
