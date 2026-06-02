import { reactive } from 'vue';

export const announcement = reactive({
    message: '',
    visible: false,
    dontShowAgain: false
});

export async function fetchAnnouncement() {
    //자동 팝업 비활성화(COC전용)
    return;
}

export function dismissAnnouncement() {
    if (announcement.dontShowAgain) {
        localStorage.setItem('announcement/dismissed', announcement.message);
    }
    announcement.visible = false;
    announcement.dontShowAgain = false;
}

export function showAnnouncement() {
    announcement.dontShowAgain = false;
    announcement.visible = true;
}
