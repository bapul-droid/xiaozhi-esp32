#include "genius_ui.h"

#include "boot_manager.h"
#include "minji_face.h"


namespace {

void StartMinjiFace(
    lv_obj_t* screen
)
{
    MinjiFace::Init(screen);
}

}  // namespace


void GeniusUI::Init(
    lv_obj_t* screen
)
{
    BootManager::Start(
        screen,
        StartMinjiFace
    );
}
