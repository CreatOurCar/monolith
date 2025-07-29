import AppLayout from '@/layout/AppLayout.vue';
import { createRouter, createWebHistory } from 'vue-router';

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      component: AppLayout,
      children: [
        {
          path: '/',
          name: 'Telemetry',
          component: () => import('@/views/telemetry.vue')
        },
        {
          path: '/viewer',
          name: 'Viewer',
          component: () => import('@/views/viewer.vue')
        },
        {
          path: '/settings',
          name: 'Settings',
          component: () => import('@/views/settings.vue')
        },
      ]
    },
    // {
    //     path: '/landing',
    //     name: 'landing',
    //     component: () => import('@/views/pages/Landing.vue')
    // },
  ]
});

export default router;
