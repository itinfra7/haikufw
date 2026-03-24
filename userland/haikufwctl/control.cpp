#include "control.hpp"

#include <stdexcept>

#ifdef __HAIKU__
#include <Drivers.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace haikufw {

#ifdef __HAIKU__

namespace {

class DeviceFd {
public:
	DeviceFd()
		:
		fFd(-1)
	{
	}

	~DeviceFd()
	{
		if (fFd >= 0)
			close(fFd);
	}

	void
	open_device()
	{
		fFd = open(HAIKUFW_DEVICE_PATH, O_RDWR);
		if (fFd < 0)
			throw std::runtime_error("unable to open " HAIKUFW_DEVICE_PATH);
	}

	int
	get() const
	{
		return fFd;
	}

private:
	int fFd;
};

} // namespace


void
load_ruleset_into_kernel(const std::vector<uint8_t>& blob)
{
	DeviceFd device;
	device.open_device();

	if (ioctl(device.get(), HAIKUFW_IOCTL_LOAD_RULESET,
			const_cast<uint8_t*>(blob.data()), blob.size()) != 0) {
		throw std::runtime_error("failed to load ruleset into kernel");
	}
}


void
clear_kernel_ruleset()
{
	DeviceFd device;
	device.open_device();

	if (ioctl(device.get(), HAIKUFW_IOCTL_CLEAR_RULESET, NULL, 0) != 0)
		throw std::runtime_error("failed to clear ruleset in kernel");
}


haikufw_status
fetch_kernel_status()
{
	DeviceFd device;
	device.open_device();

	haikufw_status status {};
	if (ioctl(device.get(), HAIKUFW_IOCTL_GET_STATUS, &status,
			sizeof(status)) != 0) {
		throw std::runtime_error("failed to fetch kernel status");
	}
	return status;
}

#else

void
load_ruleset_into_kernel(const std::vector<uint8_t>&)
{
	throw std::runtime_error("kernel control path is available only on Haiku");
}


void
clear_kernel_ruleset()
{
	throw std::runtime_error("kernel control path is available only on Haiku");
}


haikufw_status
fetch_kernel_status()
{
	throw std::runtime_error("kernel control path is available only on Haiku");
}

#endif

} // namespace haikufw
