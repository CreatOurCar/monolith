<script setup>
  import {useLayout} from '@/layout/composables/layout';
  import AppConfigurator from './AppConfigurator.vue';

  const {toggleMenu, toggleDarkMode, isDarkTheme} = useLayout();
</script>

<template>
  <div class="layout-topbar">
    <div class="layout-topbar-logo-container">
      <button class="layout-menu-button layout-topbar-action" @click="toggleMenu">
        <i class="pi pi-bars"></i>
      </button>
      <router-link to="/" class="layout-topbar-logo">
        <img src="/icon_small.png" alt="logo" class="layout-topbar-logo-image" />
        <span>monolith</span>
      </router-link>
    </div>

    <div class="layout-topbar-actions">
      <div class="layout-config-menu">
        <button type="button" class="layout-topbar-action" @click="toggleDarkMode">
          <i :class="['pi', { 'pi-moon': isDarkTheme, 'pi-sun': !isDarkTheme }]"></i>
        </button>
        <AppConfigurator />
        <button type="button" class="layout-topbar-action">
          <i class="pi pi-sitemap"></i>
          <span>Calendar</span>
        </button>
      </div>
    </div>
  </div>
</template>
