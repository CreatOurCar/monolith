<script setup>
import { ref } from 'vue';

import AppMenuItem from './AppMenuItem.vue';

const model = ref([
    {
        label: 'Main',
        items: [
            { label: 'Live Telemetry', icon: 'pi pi-fw pi-wifi', to: '/' },
            { label: 'Data Viewer', icon: 'pi pi-fw pi-chart-bar', to: '/viewer' },
            { label: 'Settings', icon: 'pi pi-fw pi-cog', to: '/settings' },
        ]
    },
    {
        label: 'Get Started',
        items: [
            {
                label: 'GitHub',
                icon: 'pi pi-fw pi-github',
                url: 'https://github.com/luftaquila/monolith',
                target: '_blank'
            },
            {
                label: 'DNF Forum',
                icon: 'pi pi-fw pi-link',
                url: 'https://dnf.luftaquila.io',
                target: '_blank'
            },
            {
                label: 'Documentation',
                icon: 'pi pi-fw pi-book',
                url: 'https://monolith.github.io',
                target: '_blank'
            },
        ]
    }
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
