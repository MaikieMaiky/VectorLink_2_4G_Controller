#ifndef VECTORLINK_APP_MAIN_H
#define VECTORLINK_APP_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Initializes the C++ application layer and creates its FreeRTOS tasks.
 *
 * This C-linkage function is the stable boundary called by CubeMX-generated C code.
 */
void VectorLink_InitializeApp(void);

#ifdef __cplusplus
}
#endif

#endif /* VECTORLINK_APP_MAIN_H */
