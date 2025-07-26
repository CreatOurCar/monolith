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
                    name: 'telemetry',
                    component: () => import('@/views/telemetry.vue')
                },
                {
                    path: '/viewer',
                    name: 'viewer',
                    component: () => import('@/views/viewer.vue')
                },
                {
                    path: '/console',
                    name: 'console',
                    component: () => import('@/views/console.vue')
                },
                {
                    path: '/settings',
                    name: 'settings',
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
