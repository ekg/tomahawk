# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
src/algorithm/load_balancer_ld.cpp 

OBJS += \
src/algorithm/load_balancer_ld.o 

CPP_DEPS += \
src/algorithm/load_balancer_ld.d 


# Each subdirectory must supply rules for building sources it contributes
src/algorithm/%.o: src/algorithm/%.cpp
	g++ $(CXXFLAGS) $(INCLUDE_PATH) -c -fmessage-length=0  -DVERSION=\"$(GIT_VERSION)\" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"


