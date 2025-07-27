<script setup>
    import {ref, reactive, onMounted} from 'vue';

    import uPlot from 'uplot';
    import 'uplot/dist/uPlot.min.css';

    const display = reactive({
        digital: true,
        analog: true,
        gyroscope: true,
        gps: true,
        syslog: true,
        system: true,
    });

    function parse(file) {
        console.log('Parsing file:', file);
    }

    const data = [
        [0, 1, 2, 3, 4],   // x 축
        [10, 20, 15, 30, 25]  // y 축
    ];

    // uPlot 옵션 예시
    const opts = {
        title: "Digital Signal",
        width: 600,
        height: 300,
        scales: {
            x: {time: false},
            y: {auto: true},
        },
        axes: [
            {stroke: "#888"},
            {stroke: "#888"},
        ],
        series: [
            {},
            {label: "Value", stroke: "#1f77b4"},
        ],
    };

    const chartEl = ref(null)
    let chart = null


    onMounted(() => {
        chart = new uPlot(opts, data, chartEl.value);
    });

</script>

<template>
    <div class="grid grid-cols-12 gap-8">
        <div class="col-span-full lg:col-span-12">
            <div class="card">
                <div class="font-semibold text-xl">File</div>
                <div class="flex justify-start mt-4 w-full">
                    <FileUpload ref="fileupload" mode="basic" accept=".log" @uploader="parse" customUpload
                        chooseIcon="pi pi-file" chooseLabel="Select" />
                </div>
            </div>

            <div class="card">
                <div class="flex items-center justify-between mb-0">
                    <div class="font-semibold text-xl">Digital</div>
                    <ToggleButton v-model="display.digital" onLabel="Hide" offLabel="Show" />
                </div>

                <div v-show="display.digital" class="flex justify-start mt-4">
                    <div ref="chartEl"></div>
                </div>
            </div>

        </div>
    </div>
</template>
