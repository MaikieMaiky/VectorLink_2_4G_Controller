#include "app_main.h"

extern "C"
{
#include "FreeRTOS.h"
#include "main.h"
#include "task.h"
}

namespace vectorlink
{ 

class Application final
{
public:
  static void Initialize()
  {
    const BaseType_t result =
        xTaskCreate(TaskEntry, "AppTask", kTaskStackDepth, nullptr, kTaskPriority, &task_handle_);

    if (result != pdPASS)
    {
      Error_Handler();
    }
  }

private:
  static constexpr uint16_t kTaskStackDepth = 256;
  static constexpr UBaseType_t kTaskPriority = tskIDLE_PRIORITY + 1;

  static void TaskEntry(void* argument)
  {
    (void)argument;

    for (;;)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  static TaskHandle_t task_handle_;
};

TaskHandle_t Application::task_handle_ = nullptr;

} // namespace vectorlink

extern "C" void VectorLink_InitializeApp(void)
{
  vectorlink::Application::Initialize();
}
