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
                    path: '/server',
                    name: 'server',
                    component: () => import('@/views/server.vue')
                },
                {
                    path: '/device',
                    name: 'device',
                    component: () => import('@/views/device.vue')
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
