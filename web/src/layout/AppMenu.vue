<script setup>
import { computed } from 'vue';

import AppMenuItem from './AppMenuItem.vue';
import { announcement, showAnnouncement } from '@/service/announcement';

const model = computed(() => [
    {
        label: 'Main',
        items: [
            { label: 'Live Telemetry', icon: 'pi pi-fw pi-wifi', to: '/' },
            { label: 'Data Viewer', icon: 'pi pi-fw pi-database', to: '/viewer' },
            { label: 'UI Configuration', icon: 'pi pi-fw pi-desktop', to: '/ui' },
            { label: 'Device Configuration', icon: 'pi pi-fw pi-cog', to: '/device' }
        ]
    },
]);
</script>

<template>
    <ul class="layout-menu">
        <template v-for="(item, i) in model" :key="item">
            <app-menu-item v-if="!item.separator" :item="item" :index="i"></app-menu-item>
            <li v-if="item.separator" class="menu-separator"></li>
        </template>
    </ul>
</template>

<style lang="scss" scoped></style>
